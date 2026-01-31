import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC
from . import ST25R3918Component, st25r3918_ns

DEPENDENCIES = ["st25r3918"]

CONF_FRAGRANCE_NAME = "fragrance_name"
CONF_CART_ID = "cart_id"
CONF_ST25R3918_ID = "st25r3918_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ST25R3918_ID): cv.use_id(ST25R3918Component),
        cv.Optional(CONF_FRAGRANCE_NAME): text_sensor.text_sensor_schema(
            icon="mdi:scent",
        ),
        cv.Optional(CONF_CART_ID): text_sensor.text_sensor_schema(
            icon="mdi:identifier",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ST25R3918_ID])

    if CONF_FRAGRANCE_NAME in config:
        sens = await text_sensor.new_text_sensor(config[CONF_FRAGRANCE_NAME])
        cg.add(parent.set_fragrance_name_sensor(sens))

    if CONF_CART_ID in config:
        sens = await text_sensor.new_text_sensor(config[CONF_CART_ID])
        cg.add(parent.set_cart_id_sensor(sens))
