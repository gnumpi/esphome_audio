"""Microphone platform implementation as ADF-Pipeline Element."""

import esphome.codegen as cg
from esphome.components import microphone
import esphome.config_validation as cv
from esphome.const import CONF_ID

from .. import (
    esp_adf_ns,
    ADFPipelineController,
    ADF_PIPELINE_CONTROLLER_SCHEMA,
    setup_pipeline_controller,
)

CODEOWNERS = ["@gnumpi"]
DEPENDENCIES = ["adf_pipeline", "microphone"]


ADFMicrophone = esp_adf_ns.class_(
    "ADFMicrophone", ADFPipelineController, microphone.Microphone, cg.Component
)

CONFIG_SCHEMA = microphone.MICROPHONE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ADFMicrophone),
    }
).extend(ADF_PIPELINE_CONTROLLER_SCHEMA)


# @coroutine_with_priority(100.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await setup_pipeline_controller(var, config)
    await microphone.register_microphone(var, config)
