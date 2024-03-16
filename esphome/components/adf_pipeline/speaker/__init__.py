"""Speaker platform implementation as ADF-Pipeline Element."""

import esphome.codegen as cg
from esphome.components import speaker
import esphome.config_validation as cv
from esphome.const import CONF_ID

from .. import (
    esp_adf_ns,
    ADFPipelineController,
    ADF_PIPELINE_CONTROLLER_SCHEMA,
    setup_pipeline_controller,
)


CODEOWNERS = ["@gnumpi"]
DEPENDENCIES = ["adf_pipeline", "speaker"]


ADFSpeaker = esp_adf_ns.class_(
    "ADFSpeaker", ADFPipelineController, speaker.Speaker, cg.Component
)

CONFIG_SCHEMA = speaker.SPEAKER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ADFSpeaker),
    }
).extend(ADF_PIPELINE_CONTROLLER_SCHEMA)


# @coroutine_with_priority(100.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await setup_pipeline_controller(var, config)
    await speaker.register_speaker(var, config)
