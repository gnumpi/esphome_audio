import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID

from .. import matrixio_ns, wb_device, wb_device_schema, register_wb_device

DEPENDENCIES = ["matrixio"]
CODEOWNERS = ["@gnumpi"]

everloop = matrixio_ns.class_("Everloop", light.AddressableLight, wb_device)

CONFIG_SCHEMA = light.ADDRESSABLE_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(everloop),
    }
).extend(wb_device_schema())


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)
    await register_wb_device(var, config)
    await cg.register_component(var, config)
