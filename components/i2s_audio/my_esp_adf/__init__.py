import esphome.codegen as cg
from esphome import pins
import esphome.config_validation as cv

from ... import my_esp_adf as esp_adf
from esphome.const import CONF_ID

from .. import (
    CONF_I2S_AUDIO_ID,
    CONF_I2S_DOUT_PIN,
    CONF_I2S_DIN_PIN,
    I2SAudioComponent,
    I2SAudioOut,
    I2SAudioIn,
    i2s_audio_ns,
) 

AUTO_LOAD = ["my_esp_adf"]
DEPENDENCIES = ["my_esp_adf","i2s_audio"]

ADFElementI2SOut = i2s_audio_ns.class_(
    "ADFElementI2SOut", esp_adf.ADFPipelineSink, esp_adf.ADFPipelineElement, I2SAudioOut, cg.Component
)

ADFElementI2SIn = i2s_audio_ns.class_(
    "ADFElementI2SIn", esp_adf.ADFPipelineSource, esp_adf.ADFPipelineElement, I2SAudioIn, cg.Component
)


CONFIG_SCHEMA_OUT = esp_adf.ADF_COMPONENT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ADFElementI2SOut),
        cv.GenerateID(CONF_I2S_AUDIO_ID): cv.use_id(I2SAudioComponent),
        cv.Required(CONF_I2S_DOUT_PIN): pins.internal_gpio_output_pin_number,
        
    }
)

CONFIG_SCHEMA_IN = esp_adf.ADF_COMPONENT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ADFElementI2SIn),
        cv.GenerateID(CONF_I2S_AUDIO_ID): cv.use_id(I2SAudioComponent),
        cv.Required(CONF_I2S_DIN_PIN): pins.internal_gpio_output_pin_number,
        
    }
)

CONFIG_SCHEMA = esp_adf.construct_pipeline_element_config_schema(CONFIG_SCHEMA_OUT, CONFIG_SCHEMA_IN)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    await cg.register_parented(var, config[CONF_I2S_AUDIO_ID])
    if config["type"] == "sink":
        cg.add(var.set_dout_pin(config[CONF_I2S_DOUT_PIN]))
    elif config["type"] == "source":
        cg.add(var.set_din_pin(config[CONF_I2S_DIN_PIN]))
    await esp_adf.register_adf_component(var, config)
