import esphome.codegen as cg
import esphome.config_validation as cv
#from esphome.components import my_esp_adf as esp_adf
from ... import my_esp_adf as esp_adf
from esphome.const import CONF_ID

from .. import (
    matrixio_ns, 
    wb_device, 
    wb_device_schema, 
    register_wb_device
  ) 

AUTO_LOAD = ["my_esp_adf"]
DEPENDENCIES = ["matrixio"]

MatrixIOStreamWriter = matrixio_ns.class_(
    "MatrixIOStreamWriter", esp_adf.ADFAudioComponent, wb_device, cg.Component
)

CONF_AUDIO_OUT = "audio_out"
CONF_VOLUME = "volume"

OutputSelector = matrixio_ns.enum("OutputSelector")
OUTPUTS = {"speakers": OutputSelector.kSpeaker, "headphone": OutputSelector.kHeadPhone}


CONFIG_SCHEMA = esp_adf.ADF_COMPONENT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(MatrixIOStreamWriter),
        cv.Optional(CONF_AUDIO_OUT, default="headphone"): cv.enum(OUTPUTS, upper=False),
        cv.Optional(CONF_VOLUME, default=80): cv.All(int, cv.Range(min=1, max=100)),
    }
).extend(wb_device_schema())

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_output(config[CONF_AUDIO_OUT]))
    cg.add(var.set_target_volume(config[CONF_VOLUME]))
    await cg.register_component(var, config)
    await register_wb_device(var, config)
    await esp_adf.register_adf_component(var, config)
