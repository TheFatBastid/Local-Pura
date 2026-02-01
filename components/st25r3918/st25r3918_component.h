#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/preferences.h"
#include "esphome/core/defines.h"

#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

// Include RFAL headers directly
#include "rfal_nfc.h"
#include "rfal_rfst25r3918.h"

#include <map>
#include <string>

namespace esphome {
namespace st25r3918 {

class ST25R3918Component : public PollingComponent {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  void loop() override;

  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_irq_pin(GPIOPin *pin) { this->irq_pin_ = pin; }
  void set_i2c_pins(int sda, int scl) { this->sda_pin_ = sda; this->scl_pin_ = scl; }
  void add_cart_name(const std::string &cart_id, const std::string &name) {
    this->configured_cart_names_[cart_id] = name;
  }

#ifdef USE_TEXT_SENSOR
  void set_fragrance_name_sensor(text_sensor::TextSensor *sensor) { this->fragrance_name_sensor_ = sensor; }
  void set_cart_id_sensor(text_sensor::TextSensor *sensor) { this->cart_id_sensor_ = sensor; }
#endif
#ifdef USE_BINARY_SENSOR
  void set_tag_present_sensor(binary_sensor::BinarySensor *sensor) { this->tag_present_sensor_ = sensor; }
#endif
#ifdef USE_SENSOR
  void set_usage_time_sensor(sensor::Sensor *sensor) { this->usage_time_sensor_ = sensor; }
  void set_scent_remaining_sensor(sensor::Sensor *sensor) { this->scent_remaining_sensor_ = sensor; }
#endif

  // Force an immediate write of usage counters to NVS (call before rebooting).
  void flush_usage() { this->save_usage_data_(); }

  // Called from YAML to indicate heater state (flush on OFF edge).
  void set_heating(bool heating) {
    if (this->heating_ && !heating) {
      this->save_usage_data_();
    }
    this->heating_ = heating;
  }

  // Getters for Home Assistant sensors
  const char *get_cart_id() const { return this->cart_id_; }
  const char *get_fragrance_name() const { return this->fragrance_name_; }
  bool is_tag_present() const { return this->tag_present_; }

 protected:
  GPIOPin *irq_pin_{nullptr};
  int sda_pin_{27};
  int scl_pin_{14};

  // RFAL objects
  RfalRfST25R3918Class *rfal_hardware_{nullptr};
  RfalNfcClass *rfal_nfc_{nullptr};

  bool initialized_{false};
  bool discovery_started_{false};
  bool tag_present_{false};

  uint8_t last_uid_[10];
  uint8_t last_uid_len_{0};

  // Pura cart info (extracted from NDEF)
  char cart_id_[32]{0};
  char cart_url_[128]{0};
  char fragrance_name_[64]{0};

  // Tag detection - only log new tags
  uint8_t last_detected_uid_[10];
  uint8_t last_detected_uid_len_{0};

  // Boot delay tracking
  uint32_t boot_time_{0};

  // Cart names configured in YAML
  std::map<std::string, std::string> configured_cart_names_;

  // Usage tracking
  std::map<std::string, uint32_t> cart_usage_seconds_;  // Runtime per cart in seconds
  uint32_t last_usage_update_{0};  // Last time we updated usage
  uint32_t accumulated_ms_{0};     // Accumulated milliseconds not yet added to seconds
  std::string active_cart_id_;     // Currently active cart for usage tracking
  bool heating_{false};            // True when heater is actively on
  ESPPreferenceObject usage_pref_;

  static constexpr uint32_t TOTAL_LIFE_SECONDS = 200 * 3600;  // ~200 hours at medium

  // Sensors
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *fragrance_name_sensor_{nullptr};
  text_sensor::TextSensor *cart_id_sensor_{nullptr};
#endif
#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *tag_present_sensor_{nullptr};
#endif
#ifdef USE_SENSOR
  sensor::Sensor *usage_time_sensor_{nullptr};
  sensor::Sensor *scent_remaining_sensor_{nullptr};
#endif

  // Internal methods
  bool init_rfal_();
  void handle_nfc_state_(rfalNfcState state, rfalNfcDevice *device);
  void read_nfcv_memory_(rfalNfcDevice *device, bool is_pura_cart);
  void publish_sensors_();
  void update_usage_time_();
  void load_usage_data_();
  void save_usage_data_();

  // Static callback for RFAL
  static void nfc_callback_(rfalNfcState state);
  static ST25R3918Component *instance_;
};

}  // namespace st25r3918
}  // namespace esphome
