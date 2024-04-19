"""Media-Player platform implementation as ADF-Pipeline Element."""

import esphome.codegen as cg
from esphome.components import media_player
import esphome.config_validation as cv
from esphome.const import CONF_ID

from .. import (
    esp_adf_ns,
    ADFPipelineController,
    ADF_PIPELINE_CONTROLLER_SCHEMA,
    setup_pipeline_controller,
)

CODEOWNERS = ["@gnumpi"]
DEPENDENCIES = ["adf_pipeline", "media_player"]
CONF_RESTORE_VOLUME = "restore_volume"


ADFMediaPlayer = esp_adf_ns.class_(
    "ADFMediaPlayer", ADFPipelineController, media_player.MediaPlayer, cg.Component
)

CONFIG_SCHEMA = media_player.MEDIA_PLAYER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ADFMediaPlayer),
        cv.Optional(CONF_RESTORE_VOLUME, default=False): cv.boolean,
    }
).extend(ADF_PIPELINE_CONTROLLER_SCHEMA)


# @coroutine_with_priority(100.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await setup_pipeline_controller(var, config)
    cg.add(var.set_restore_volume(config[CONF_RESTORE_VOLUME]))
    await media_player.register_media_player(var, config)
