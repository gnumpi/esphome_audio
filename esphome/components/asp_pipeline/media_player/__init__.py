import esphome.codegen as cg
from esphome.components import media_player
import esphome.config_validation as cv
from esphome.core import coroutine_with_priority

from esphome.const import CONF_ID, CONF_MODE

from ...matrixio import my_esp_adf as matrix_io_adf

CODEOWNERS = ["@gnumpi"]
DEPENDENCIES = ["my_esp_adf", "media_player"]

from .. import (
    esp_adf_ns,
    ADFPipelineComponent,
    CONF_ADF_COMP_ID,
    ADF_COMPONENT_SCHEMA,
    add_pipeline_elements
)

ADFMediaPlayer = esp_adf_ns.class_(
    "ADFMediaPlayer", ADFPipelineComponent, media_player.MediaPlayer, cg.Component
)

CONFIG_SCHEMA = media_player.MEDIA_PLAYER_SCHEMA.extend(
    {
      cv.GenerateID(): cv.declare_id(ADFMediaPlayer),  
      #cv.GenerateID(CONF_ADF_COMP_ID) : cv.use_id(ADFAudioComponent)
    }
).extend(ADF_COMPONENT_SCHEMA)

#@coroutine_with_priority(100.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await add_pipeline_elements(var, config)
    
    #comp = await cg.get_variable(config[CONF_ADF_COMP_ID])
    #cg.add( var.add_to_pipeline(comp) )
    
    await media_player.register_media_player(var, config)

    #await cg.register_parented(var, config[CONF_I2S_AUDIO_ID])
