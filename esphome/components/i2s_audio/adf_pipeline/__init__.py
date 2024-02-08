import esphome.codegen as cg
from esphome import pins
import esphome.config_validation as cv

from ... import adf_pipeline as esp_adf
from esphome.const import CONF_ID, CONF_CHANNEL

from .. import (
    CONF_I2S_AUDIO_ID,
    CONF_I2S_DOUT_PIN,
    CONF_I2S_DIN_PIN,
    I2SAudioComponent,
    I2SAudioOut,
    I2SAudioIn,
    i2s_audio_ns,
)

CODEOWNERS = ["@gnumpi"]
AUTO_LOAD = ["adf_pipeline"]
DEPENDENCIES = ["adf_pipeline", "i2s_audio"]

CONF_SAMPLE_RATE = "sample_rate"
CONF_BITS_PER_SAMPLE = "bits_per_sample"


ADFElementI2SOut = i2s_audio_ns.class_(
    "ADFElementI2SOut",
    esp_adf.ADFPipelineSink,
    esp_adf.ADFPipelineElement,
    I2SAudioOut,
    cg.Component,
)

ADFElementI2SIn = i2s_audio_ns.class_(
    "ADFElementI2SIn",
    esp_adf.ADFPipelineSource,
    esp_adf.ADFPipelineElement,
    I2SAudioIn,
    cg.Component,
)

i2s_channel_fmt_t = cg.global_ns.enum("i2s_channel_fmt_t")
CHANNELS = {
    # Only load data in left channel (mono mode)
    "left": i2s_channel_fmt_t.I2S_CHANNEL_FMT_ONLY_LEFT,
    # Only load data in right channel (mono mode)
    "right": i2s_channel_fmt_t.I2S_CHANNEL_FMT_ONLY_RIGHT,  #
    # Separated left and right channel
    "right_left": i2s_channel_fmt_t.I2S_CHANNEL_FMT_RIGHT_LEFT,
    # Load right channel data in both two channels
    "all_right": i2s_channel_fmt_t.I2S_CHANNEL_FMT_ALL_RIGHT,
    # Load left channel data in both two channels
    "all_left": i2s_channel_fmt_t.I2S_CHANNEL_FMT_ALL_LEFT,
}

i2s_bits_per_sample_t = cg.global_ns.enum("i2s_bits_per_sample_t")
BITS_PER_SAMPLE = {
    16: i2s_bits_per_sample_t.I2S_BITS_PER_SAMPLE_16BIT,
    32: i2s_bits_per_sample_t.I2S_BITS_PER_SAMPLE_32BIT,
}

_validate_bits = cv.float_with_unit("bits", "bit")

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
        cv.Optional(CONF_CHANNEL, default="right"): cv.enum(CHANNELS),
        cv.Optional(CONF_SAMPLE_RATE, default=16000): cv.int_range(min=1),
        cv.Optional(CONF_BITS_PER_SAMPLE, default="16bit"): cv.All(
            _validate_bits, cv.enum(BITS_PER_SAMPLE)
        ),
    }
)

CONFIG_SCHEMA = esp_adf.construct_pipeline_element_config_schema(
    CONFIG_SCHEMA_OUT, CONFIG_SCHEMA_IN
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    await cg.register_parented(var, config[CONF_I2S_AUDIO_ID])
    if config["type"] == "sink":
        cg.add(var.set_dout_pin(config[CONF_I2S_DOUT_PIN]))
    elif config["type"] == "source":
        cg.add(var.set_din_pin(config[CONF_I2S_DIN_PIN]))
        cg.add(var.set_channel(config[CONF_CHANNEL]))
        cg.add(var.set_sample_rate(config[CONF_SAMPLE_RATE]))
        cg.add(var.set_bits_per_sample(config[CONF_BITS_PER_SAMPLE]))

    await esp_adf.register_adf_component(var, config)
