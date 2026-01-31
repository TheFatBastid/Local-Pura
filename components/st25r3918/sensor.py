import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    UNIT_HOUR,
    UNIT_PERCENT,
    ICON_TIMER,
    STATE_CLASS_TOTAL_INCREASING,
    STATE_CLASS_MEASUREMENT,
)
from . import ST25R3918Component, st25r3918_ns

DEPENDENCIES = ["st25r3918"]

CONF_USAGE_TIME = "usage_time"
CONF_SCENT_REMAINING = "scent_remaining"
CONF_ST25R3918_ID = "st25r3918_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ST25R3918_ID): cv.use_id(ST25R3918Component),
        cv.Optional(CONF_USAGE_TIME): sensor.sensor_schema(
            unit_of_measurement=UNIT_HOUR,
            icon=ICON_TIMER,
            accuracy_decimals=1,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_SCENT_REMAINING): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            icon="mdi:bottle-tonic-outline",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ST25R3918_ID])

    if CONF_USAGE_TIME in config:
        sens = await sensor.new_sensor(config[CONF_USAGE_TIME])
        cg.add(parent.set_usage_time_sensor(sens))

    if CONF_SCENT_REMAINING in config:
        sens = await sensor.new_sensor(config[CONF_SCENT_REMAINING])
        cg.add(parent.set_scent_remaining_sensor(sens))
