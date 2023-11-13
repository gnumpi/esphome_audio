import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import speaker
from esphome.const import CONF_ID

from .. import (
    matrixio_ns,
    wb_device,
    wb_device_schema,
    register_wb_device
)

matrix_speaker = matrixio_ns.class_("Speaker", speaker.Speaker, wb_device, cg.Component)

CONFIG_SCHEMA = speaker.SPEAKER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(matrix_speaker) 
    }
).extend(wb_device_schema())

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await register_wb_device(var,config)
    await speaker.register_speaker(var, config)
