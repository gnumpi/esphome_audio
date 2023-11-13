import esphome.config_validation as cv
import esphome.codegen as cg

from esphome.const import CONF_ID
from esphome.components import microphone

from .. import (
    matrixio_ns,
    wb_device,
    wb_device_schema,
    register_wb_device
)

CODEOWNERS = ["@gnumpi"]

mics = matrixio_ns.class_("Microphone", microphone.Microphone, wb_device, cg.Component)

CONFIG_SCHEMA = microphone.MICROPHONE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(mics) 
    }
).extend(wb_device_schema())

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await register_wb_device(var,config)
    await microphone.register_microphone(var, config)
    
    
