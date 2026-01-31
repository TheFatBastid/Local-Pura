import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import DEVICE_CLASS_PRESENCE
from . import ST25R3918Component, st25r3918_ns

DEPENDENCIES = ["st25r3918"]

CONF_TAG_PRESENT = "tag_present"
CONF_ST25R3918_ID = "st25r3918_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ST25R3918_ID): cv.use_id(ST25R3918Component),
        cv.Optional(CONF_TAG_PRESENT): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PRESENCE,
            icon="mdi:nfc-variant",
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ST25R3918_ID])

    if CONF_TAG_PRESENT in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_TAG_PRESENT])
        cg.add(parent.set_tag_present_sensor(sens))
