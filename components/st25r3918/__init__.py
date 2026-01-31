import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID, CONF_NAME

CODEOWNERS = ["@TheFatBastid"]
MULTI_CONF = False

CONF_IRQ_PIN = "irq_pin"
CONF_SDA_PIN = "sda_pin"
CONF_SCL_PIN = "scl_pin"
CONF_CARTS = "carts"
CONF_CART_ID = "cart_id"

st25r3918_ns = cg.esphome_ns.namespace("st25r3918")
ST25R3918Component = st25r3918_ns.class_(
    "ST25R3918Component", cg.PollingComponent
)

CART_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_CART_ID): cv.string,
        cv.Required(CONF_NAME): cv.string,
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ST25R3918Component),
            cv.Required(CONF_IRQ_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_SDA_PIN, default=27): cv.int_,
            cv.Optional(CONF_SCL_PIN, default=14): cv.int_,
            cv.Optional(CONF_CARTS, default=[]): cv.ensure_list(CART_SCHEMA),
        }
    )
    .extend(cv.polling_component_schema("500ms"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    irq_pin = await cg.gpio_pin_expression(config[CONF_IRQ_PIN])
    cg.add(var.set_irq_pin(irq_pin))
    cg.add(var.set_i2c_pins(config[CONF_SDA_PIN], config[CONF_SCL_PIN]))

    # Add configured cart names
    for cart in config[CONF_CARTS]:
        cg.add(var.add_cart_name(cart[CONF_CART_ID], cart[CONF_NAME]))
