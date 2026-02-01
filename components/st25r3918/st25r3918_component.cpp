#include "st25r3918_component.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/defines.h"
#include "rfal_nfcv.h"

#include <Wire.h>
#include <cstring>
#include <Preferences.h>

namespace esphome {
namespace st25r3918 {

static const char *const TAG = "st25r3918";

// Static instance for callback
ST25R3918Component *ST25R3918Component::instance_ = nullptr;

// Static callback function for RFAL
void ST25R3918Component::nfc_callback_(rfalNfcState state) {
  if (instance_ != nullptr && instance_->rfal_nfc_ != nullptr) {
    rfalNfcDevice *dev = nullptr;
    instance_->rfal_nfc_->rfalNfcGetActiveDevice(&dev);
    instance_->handle_nfc_state_(state, dev);
  }
}

void ST25R3918Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up ST25R3918...");

  instance_ = this;
  this->boot_time_ = millis();

  // Setup IRQ pin
  if (this->irq_pin_ != nullptr) {
    this->irq_pin_->setup();
  }

  // Log configured cart names
  if (!this->configured_cart_names_.empty()) {
    ESP_LOGCONFIG(TAG, "Configured %d cart name(s)", this->configured_cart_names_.size());
  }

  // Load usage data from flash
  this->load_usage_data_();
}

void ST25R3918Component::loop() {
  if (!this->initialized_ || this->rfal_nfc_ == nullptr) {
    return;
  }

  // Run the RFAL worker to process NFC state machine
  this->rfal_nfc_->rfalNfcWorker();
}

void ST25R3918Component::update() {
  // Wait 2 seconds after boot before initializing
  if (!this->initialized_) {
    uint32_t elapsed = millis() - this->boot_time_;
    if (elapsed < 2000) {
      return;
    }

    ESP_LOGI(TAG, "Initializing ST25R3918...");
    if (this->init_rfal_()) {
      this->initialized_ = true;
      ESP_LOGI(TAG, "ST25R3918 initialized - ready for NFC tags");
    } else {
      ESP_LOGE(TAG, "ST25R3918 initialization failed - will retry");
      this->boot_time_ = millis() - 2000 + 5000;
    }
    return;
  }

  // Update usage time tracking
  this->update_usage_time_();

  // Publish sensor values
  this->publish_sensors_();
}

bool ST25R3918Component::init_rfal_() {
  // Initialize Arduino Wire library with configured pins
  ESP_LOGI(TAG, "Initializing Wire with SDA=%d, SCL=%d...", this->sda_pin_, this->scl_pin_);
  Wire.begin(this->sda_pin_, this->scl_pin_);
  Wire.setClock(100000);  // 100kHz

  // Scan I2C bus
  ESP_LOGI(TAG, "Scanning I2C bus...");
  bool found = false;
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      ESP_LOGI(TAG, "  Device found at 0x%02X", addr);
      if (addr == 0x50) {
        found = true;
      }
    }
  }

  if (!found) {
    ESP_LOGE(TAG, "ST25R3918 not found at address 0x50");
    return false;
  }

  // Create RFAL objects
  // Get IRQ pin number (required for interrupt-based operation)
  int irq_pin_num = -1;
  if (this->irq_pin_ != nullptr) {
    // Cast to InternalGPIOPin to access the pin number
    auto *internal_pin = static_cast<InternalGPIOPin *>(this->irq_pin_);
    irq_pin_num = internal_pin->get_pin();
    ESP_LOGI(TAG, "Using IRQ pin: %d", irq_pin_num);
  } else {
    ESP_LOGW(TAG, "No IRQ pin configured - interrupt handling may not work!");
  }

  ESP_LOGD(TAG, "Creating RFAL hardware instance...");
  this->rfal_hardware_ = new RfalRfST25R3918Class(&Wire, irq_pin_num);

  ESP_LOGD(TAG, "Creating RFAL NFC instance...");
  this->rfal_nfc_ = new RfalNfcClass(this->rfal_hardware_);

  // Initialize RFAL
  ESP_LOGD(TAG, "Initializing RFAL NFC stack...");
  ReturnCode err = this->rfal_nfc_->rfalNfcInitialize();
  if (err != ERR_NONE) {
    ESP_LOGE(TAG, "rfalNfcInitialize failed with error: %d", err);
    return false;
  }

  ESP_LOGI(TAG, "RFAL NFC stack initialized!");

  // Configure discovery parameters
  rfalNfcDiscoverParam discParam;
  memset(&discParam, 0, sizeof(discParam));

  discParam.compMode = RFAL_COMPLIANCE_MODE_NFC;
  discParam.devLimit = 1;
  discParam.nfcfBR = RFAL_BR_212;
  discParam.ap2pBR = RFAL_BR_424;

  // Enable ISO15693 (NFC-V) for Pura carts - this is the primary protocol
  // Also enable NFC-A for testing with common cards
  // Only poll modes, no listen modes (we're a reader, not a tag)
  discParam.techs2Find = (RFAL_NFC_POLL_TECH_A | RFAL_NFC_POLL_TECH_V);

  discParam.GBLen = RFAL_NFCDEP_GB_MAX_LEN;
  discParam.notifyCb = nfc_callback_;
  discParam.totalDuration = 2000U;  // Increased from 1000ms
  discParam.wakeupEnabled = false;
  discParam.wakeupConfigDefault = true;

  ESP_LOGI(TAG, "Discovery config: techs2Find=0x%04X, duration=%dms",
           discParam.techs2Find, discParam.totalDuration);

  // Start discovery
  ESP_LOGD(TAG, "Starting NFC discovery...");
  err = this->rfal_nfc_->rfalNfcDiscover(&discParam);
  if (err != ERR_NONE) {
    ESP_LOGE(TAG, "rfalNfcDiscover failed with error: %d", err);
    return false;
  }

  this->discovery_started_ = true;
  ESP_LOGI(TAG, "NFC discovery started - ready for tags");

  return true;
}

void ST25R3918Component::handle_nfc_state_(rfalNfcState state, rfalNfcDevice *nfc_dev) {
  switch (state) {
    case RFAL_NFC_STATE_ACTIVATED:
      if (nfc_dev != nullptr) {
        this->tag_present_ = true;

        // Store UID
        this->last_uid_len_ = nfc_dev->nfcidLen;
        if (this->last_uid_len_ > sizeof(this->last_uid_)) {
          this->last_uid_len_ = sizeof(this->last_uid_);
        }
        memcpy(this->last_uid_, nfc_dev->nfcid, this->last_uid_len_);

        // Check if this is a different tag than last time
        bool same_tag = (this->last_uid_len_ == this->last_detected_uid_len_) &&
                        (memcmp(this->last_uid_, this->last_detected_uid_, this->last_uid_len_) == 0);

        // Only process if it's a NEW/different tag
        if (!same_tag) {
          // Save usage data for previous cart if applicable
          if (!this->active_cart_id_.empty() && this->active_cart_id_ != this->cart_id_) {
            this->save_usage_data_();
          }

          // Check for ST manufacturer (Pura carts)
          bool is_pura_cart = (nfc_dev->type == RFAL_NFC_LISTEN_TYPE_NFCV &&
                               nfc_dev->nfcidLen >= 8 &&
                               nfc_dev->nfcid[7] == 0xE0 &&
                               nfc_dev->nfcid[6] == 0x02);

          // Read tag memory for NFC-V tags
          if (nfc_dev->type == RFAL_NFC_LISTEN_TYPE_NFCV && this->rfal_nfc_ != nullptr) {
            this->read_nfcv_memory_(nfc_dev, is_pura_cart);
          }

          // Log the detection with fragrance name if available
          if (is_pura_cart && this->fragrance_name_[0] != '\0') {
            ESP_LOGI(TAG, "Pura cart detected: %s", this->fragrance_name_);
            // Set active cart for usage tracking
            this->active_cart_id_ = this->cart_id_;
          } else if (is_pura_cart) {
            ESP_LOGI(TAG, "Pura cart detected: %s (not configured)", this->cart_id_);
            this->active_cart_id_ = this->cart_id_;
          } else {
            // Format UID for non-Pura tags
            char uid_str[32] = {0};
            for (int i = 0; i < this->last_uid_len_ && i < 10; i++) {
              sprintf(uid_str + (i * 3), "%02X:", this->last_uid_[i]);
            }
            if (this->last_uid_len_ > 0) {
              uid_str[this->last_uid_len_ * 3 - 1] = '\0';
            }
            ESP_LOGI(TAG, "NFC tag detected: %s", uid_str);
            this->active_cart_id_.clear();
          }

          // Update last detected tag
          memcpy(this->last_detected_uid_, this->last_uid_, this->last_uid_len_);
          this->last_detected_uid_len_ = this->last_uid_len_;
        }

        // Deactivate and restart discovery
        if (this->rfal_nfc_ != nullptr) {
          this->rfal_nfc_->rfalNfcDeactivate(true);
        }
      }
      break;

    case RFAL_NFC_STATE_START_DISCOVERY:
      // Note: tag_present_ stays true because we immediately restart discovery
      // The cart is considered "removed" when a different cart is detected
      // or when we clear the detected UID (future: timeout based removal)
      break;

    default:
      break;
  }
}

void ST25R3918Component::read_nfcv_memory_(rfalNfcDevice *nfc_dev, bool is_pura_cart) {
  ReturnCode err;
  uint8_t rxBuf[64];
  uint16_t rcvLen;

  // Clear previous cart info
  this->cart_id_[0] = '\0';
  this->cart_url_[0] = '\0';
  this->fragrance_name_[0] = '\0';

  // Buffer to accumulate NDEF data for URL extraction
  uint8_t ndefData[64];
  int ndefLen = 0;

  // Read first 16 blocks of memory (4 bytes per block = 64 bytes total)
  for (uint8_t block = 0; block < 16; block++) {
    memset(rxBuf, 0, sizeof(rxBuf));
    err = this->rfal_nfc_->rfalNfcvPollerReadSingleBlock(
        RFAL_NFCV_REQ_FLAG_DEFAULT,
        nfc_dev->dev.nfcv.InvRes.UID,
        block,
        rxBuf, sizeof(rxBuf), &rcvLen);

    if (err == ERR_NONE && rcvLen > 1) {
      int dataLen = rcvLen - 1;
      if (dataLen > 4) dataLen = 4;
      if (ndefLen + dataLen <= (int)sizeof(ndefData)) {
        memcpy(ndefData + ndefLen, rxBuf + 1, dataLen);
        ndefLen += dataLen;
      }
    }
  }

  // Parse NDEF message to extract URL (only for Pura carts)
  if (ndefLen > 0 && is_pura_cart) {
    // Find NDEF message TLV (type 0x03) after 4-byte Capability Container
    int idx = 4;
    if (idx < ndefLen && ndefData[idx] == 0x03) {
      idx += 2;  // Skip TLV type and length

      // Parse NDEF record header
      if (idx + 4 < ndefLen) {
        idx++;  // Skip header byte
        idx++;  // Skip type length
        uint8_t payloadLen = ndefData[idx++];
        uint8_t recordType = ndefData[idx++];

        // Check for URI record (type 'U' = 0x55)
        if (recordType == 0x55 && idx < ndefLen) {
          uint8_t uriCode = ndefData[idx++];

          const char *uriPrefix = "";
          if (uriCode == 0x02) uriPrefix = "https://www.";
          else if (uriCode == 0x01) uriPrefix = "http://www.";

          // Extract URI payload
          int uriLen = payloadLen - 1;
          if (uriLen > 0 && idx + uriLen <= ndefLen) {
            snprintf(this->cart_url_, sizeof(this->cart_url_), "%s", uriPrefix);
            int prefixLen = strlen(this->cart_url_);

            for (int i = 0; i < uriLen && (prefixLen + i) < (int)sizeof(this->cart_url_) - 1; i++) {
              this->cart_url_[prefixLen + i] = ndefData[idx + i];
            }

            // Extract cart ID from URL (format: pura.com/ss?d=CARTID.yyy.CHECKSUM)
            char *idStart = strstr(this->cart_url_, "?d=");
            if (idStart != nullptr) {
              idStart += 3;
              char *dotPos = strchr(idStart, '.');
              if (dotPos != nullptr) {
                int idLen = dotPos - idStart;
                strncpy(this->cart_id_, idStart, (idLen < 31) ? idLen : 31);
                this->cart_id_[31] = '\0';
              }
            }
          }
        }
      }
    }

    // Look up fragrance name from YAML config
    if (this->cart_id_[0] != '\0') {
      auto it = this->configured_cart_names_.find(this->cart_id_);
      if (it != this->configured_cart_names_.end()) {
        strncpy(this->fragrance_name_, it->second.c_str(), sizeof(this->fragrance_name_) - 1);
        this->fragrance_name_[sizeof(this->fragrance_name_) - 1] = '\0';
      }
    }
  }
}

void ST25R3918Component::publish_sensors_() {
#ifdef USE_TEXT_SENSOR
  if (this->fragrance_name_sensor_ != nullptr) {
    std::string name = this->fragrance_name_[0] != '\0' ? this->fragrance_name_ : "";
    if (this->fragrance_name_sensor_->state != name) {
      this->fragrance_name_sensor_->publish_state(name);
    }
  }
  if (this->cart_id_sensor_ != nullptr) {
    std::string id = this->cart_id_[0] != '\0' ? this->cart_id_ : "";
    if (this->cart_id_sensor_->state != id) {
      this->cart_id_sensor_->publish_state(id);
    }
  }
#endif
#ifdef USE_BINARY_SENSOR
  if (this->tag_present_sensor_ != nullptr) {
    this->tag_present_sensor_->publish_state(this->tag_present_);
  }
#endif
#ifdef USE_SENSOR
  if (!this->active_cart_id_.empty()) {
    auto it = this->cart_usage_seconds_.find(this->active_cart_id_);
    uint32_t usage_seconds = (it != this->cart_usage_seconds_.end()) ? it->second : 0;
    float hours = usage_seconds / 3600.0f;

    if (this->usage_time_sensor_ != nullptr) {
      if (std::abs(this->usage_time_sensor_->state - hours) > 0.01f || std::isnan(this->usage_time_sensor_->state)) {
        this->usage_time_sensor_->publish_state(hours);
      }
    }

    if (this->scent_remaining_sensor_ != nullptr) {
      float remaining = 100.0f * (1.0f - (float)usage_seconds / (float)TOTAL_LIFE_SECONDS);
      if (remaining < 0.0f) remaining = 0.0f;
      if (remaining > 100.0f) remaining = 100.0f;
      if (std::abs(this->scent_remaining_sensor_->state - remaining) > 0.1f || std::isnan(this->scent_remaining_sensor_->state)) {
        this->scent_remaining_sensor_->publish_state(remaining);
      }
    }
  }
#endif
}

void ST25R3918Component::update_usage_time_() {
  uint32_t now = millis();

  // Only track if a cart is present AND heater is on
  if (this->heating_ && this->tag_present_ && !this->active_cart_id_.empty()) {
    // Calculate elapsed time since last update
    if (this->last_usage_update_ > 0) {
      uint32_t elapsed_ms = now - this->last_usage_update_;

      // Accumulate milliseconds, convert to seconds when we have enough
      this->accumulated_ms_ += elapsed_ms;
      uint32_t elapsed_seconds = this->accumulated_ms_ / 1000;
      this->accumulated_ms_ %= 1000;  // Keep remainder

      if (elapsed_seconds > 0) {
        this->cart_usage_seconds_[this->active_cart_id_] += elapsed_seconds;

        // Save to flash every 60 seconds
        static uint32_t last_save = 0;
        if (now - last_save > 60000) {
          this->save_usage_data_();
          last_save = now;
        }
      }
    }
  }

  this->last_usage_update_ = now;
}

void ST25R3918Component::load_usage_data_() {
  Preferences prefs;
  if (prefs.begin("pura_usage", true)) {  // true = read-only
    // Load usage for each configured cart
    for (const auto &pair : this->configured_cart_names_) {
      uint32_t seconds = prefs.getUInt(pair.first.c_str(), 0);
      if (seconds > 0) {
        this->cart_usage_seconds_[pair.first] = seconds;
        ESP_LOGD(TAG, "Loaded usage for %s: %.1f hours", pair.second.c_str(), seconds / 3600.0f);
      }
    }
    prefs.end();
  }
}

void ST25R3918Component::save_usage_data_() {
  Preferences prefs;
  if (prefs.begin("pura_usage", false)) {
    for (const auto &pair : this->cart_usage_seconds_) {
      prefs.putUInt(pair.first.c_str(), pair.second);
    }
    prefs.end();
    ESP_LOGD(TAG, "Saved usage data to flash");
  }
}

void ST25R3918Component::dump_config() {
  ESP_LOGCONFIG(TAG, "ST25R3918 NFC Reader:");
  ESP_LOGCONFIG(TAG, "  SDA Pin: %d", this->sda_pin_);
  ESP_LOGCONFIG(TAG, "  SCL Pin: %d", this->scl_pin_);
  LOG_PIN("  IRQ Pin: ", this->irq_pin_);
  LOG_UPDATE_INTERVAL(this);
  if (this->initialized_) {
    ESP_LOGCONFIG(TAG, "  Status: Initialized");
  } else {
    ESP_LOGCONFIG(TAG, "  Status: Not initialized");
  }

  // Log configured carts and their usage
  for (const auto &pair : this->configured_cart_names_) {
    auto usage_it = this->cart_usage_seconds_.find(pair.first);
    float hours = (usage_it != this->cart_usage_seconds_.end()) ? usage_it->second / 3600.0f : 0.0f;
    ESP_LOGCONFIG(TAG, "  Cart: %s = %s (%.1f hrs)", pair.first.c_str(), pair.second.c_str(), hours);
  }
}

}  // namespace st25r3918
}  // namespace esphome
