import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32", "spi"]
CODEOWNERS = ["@gnumpi"]

CONF_MATRIXIO_ID = "conf_matrixio_id"

matrixio_ns = cg.esphome_ns.namespace("matrixio")
matrixio_wb = matrixio_ns.class_("WishboneBus", cg.Component, spi.SPIDevice)

# Wishbone Bus

CONFIG_SCHEMA = spi.spi_device_schema(True, "8MHz").extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(matrixio_wb),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)


# Wishbone Devices

wb_device = matrixio_ns.class_("WishboneDevice")


def wb_device_schema():
    schema = {cv.GenerateID(CONF_MATRIXIO_ID): cv.use_id(matrixio_wb)}
    return cv.Schema(schema)


async def register_wb_device(var, config):
    parent = await cg.get_variable(config[CONF_MATRIXIO_ID])
    cg.add(var.set_wishbone_bus(parent))
