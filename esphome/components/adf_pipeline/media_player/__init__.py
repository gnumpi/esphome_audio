"""Media-Player platform implementation as ADF-Pipeline Element."""

import esphome.codegen as cg
from esphome.components import media_player
import esphome.config_validation as cv
from esphome.const import CONF_NUM_CHANNELS, CONF_ID, __version__ as ESPHOME_VERSION


from .. import (
    esp_adf_ns,
    ADFPipelineController,
    ADF_PIPELINE_CONTROLLER_SCHEMA,
    setup_pipeline_controller,
)

CODEOWNERS = ["@gnumpi"]
DEPENDENCIES = ["adf_pipeline", "media_player"]


ADFMediaPlayer = esp_adf_ns.class_(
    "ADFMediaPlayer", ADFPipelineController, media_player.MediaPlayer, cg.Component
)


ADFCodec = esp_adf_ns.enum("ADFCodec", is_class=True)
CODECS = {"auto": "ADFCodec.AUTO", "MP3": ADFCodec.MP3}

CONF_BITS_PER_SAMPLE = "bits_per_sample"
CONF_CODEC = "codec"
CONF_SAMPLE_RATE = "sample_rate"
CONF_ANNOUNCEMENT_AUDIO = "announcement_audio"

FIXED_AUDIO_SETTINGS_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_CODEC, default="MP3"): cv.enum(CODECS),
        cv.Required(CONF_SAMPLE_RATE): cv.int_range(min=1),
        cv.Required(CONF_BITS_PER_SAMPLE): cv.int_range(min=1),
        cv.Required(CONF_NUM_CHANNELS): cv.int_range(min=1),
    }
)

CONFIG_SCHEMA = media_player.MEDIA_PLAYER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ADFMediaPlayer),
        cv.Optional(CONF_ANNOUNCEMENT_AUDIO): FIXED_AUDIO_SETTINGS_SCHEMA,
        cv.Optional(CONF_CODEC, default="MP3"): cv.enum(CODECS),
    }
).extend(ADF_PIPELINE_CONTROLLER_SCHEMA)


def check_version() -> bool:
    version_parts = ESPHOME_VERSION.split(".")
    if int(version_parts[0]) > 2024 or (
        int(version_parts[0]) == 2024 and int(version_parts[1]) >= 5
    ):
        return True
    return False


# @coroutine_with_priority(100.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    if check_version():
        cg.add_define("MP_ANNOUNCE")
        if CONF_ANNOUNCEMENT_AUDIO in config:
            caa = config[CONF_ANNOUNCEMENT_AUDIO]
            cg.add(
                var.set_announce_base_track(
                    caa[CONF_CODEC],
                    caa[CONF_SAMPLE_RATE],
                    caa[CONF_BITS_PER_SAMPLE],
                    caa[CONF_NUM_CHANNELS],
                )
            )

    if config[CONF_CODEC] == "auto":
        cg.add_define("ESP_AUTO_DECODER")

    await cg.register_component(var, config)
    await setup_pipeline_controller(var, config)
    await media_player.register_media_player(var, config)
