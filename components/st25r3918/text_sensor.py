import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC
from . import ST25R3918Component, st25r3918_ns

DEPENDENCIES = ["st25r3918"]

CONF_FRAGRANCE_NAME = "fragrance_name"
CONF_CART_ID = "cart_id"
CONF_CART_ID_BAY1 = "cart_id_bay1"
CONF_CART_ID_BAY2 = "cart_id_bay2"
CONF_FRAGRANCE_BAY1 = "fragrance_name_bay1"
CONF_FRAGRANCE_BAY2 = "fragrance_name_bay2"
CONF_ST25R3918_ID = "st25r3918_id"


def _id_ts():
    return text_sensor.text_sensor_schema(
        icon="mdi:identifier", entity_category=ENTITY_CATEGORY_DIAGNOSTIC
    )


def _frag_ts():
    return text_sensor.text_sensor_schema(icon="mdi:scent")


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ST25R3918_ID): cv.use_id(ST25R3918Component),
        cv.Optional(CONF_FRAGRANCE_NAME): _frag_ts(),
        cv.Optional(CONF_CART_ID): _id_ts(),
        cv.Optional(CONF_CART_ID_BAY1): _id_ts(),
        cv.Optional(CONF_CART_ID_BAY2): _id_ts(),
        cv.Optional(CONF_FRAGRANCE_BAY1): _frag_ts(),
        cv.Optional(CONF_FRAGRANCE_BAY2): _frag_ts(),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ST25R3918_ID])

    if CONF_FRAGRANCE_NAME in config:
        cg.add(parent.set_fragrance_name_sensor(await text_sensor.new_text_sensor(config[CONF_FRAGRANCE_NAME])))
    if CONF_CART_ID in config:
        cg.add(parent.set_cart_id_sensor(await text_sensor.new_text_sensor(config[CONF_CART_ID])))

    for key, bay in [(CONF_CART_ID_BAY1, 0), (CONF_CART_ID_BAY2, 1)]:
        if key in config:
            cg.add(parent.set_cart_id_bay_sensor(bay, await text_sensor.new_text_sensor(config[key])))
    for key, bay in [(CONF_FRAGRANCE_BAY1, 0), (CONF_FRAGRANCE_BAY2, 1)]:
        if key in config:
            cg.add(parent.set_fragrance_bay_sensor(bay, await text_sensor.new_text_sensor(config[key])))
