"""ADF-Pipeline platform implementation of I2S controller (TX and RX)."""

from esphome import pins

from esphome.components import i2c

import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.const import CONF_ID, CONF_CHANNEL

from ... import adf_pipeline as esp_adf
from ... import i2s_audio as i2s

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
CONF_PDM = "pdm"
CONF_USE_APLL = "use_apll"
CONF_IC_CNTRL = "ic"


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

ADFI2SOut_AW88298 = i2s_audio_ns.class_(
    "ADFI2SOut_AW88298", ADFElementI2SOut, i2c.I2CDevice
)

ADFI2SIn_ES7210 = i2s_audio_ns.class_("ADFI2SIn_ES7210", ADFElementI2SIn, i2c.I2CDevice)


ICS = ["AW88298"]

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
    24: i2s_bits_per_sample_t.I2S_BITS_PER_SAMPLE_24BIT,
    32: i2s_bits_per_sample_t.I2S_BITS_PER_SAMPLE_32BIT,
}

_validate_bits = cv.float_with_unit("bits", "bit")

CONF_ID_AW88298 = "conf_id_aw88298"


CONFIG_SCHEMA_IN = i2s.CONFIG_SCHEMA_I2S_IN.extend(
    {
        cv.GenerateID(): cv.declare_id(ADFElementI2SIn),
        cv.GenerateID(CONF_I2S_AUDIO_ID): cv.use_id(I2SAudioComponent),
        cv.Required(CONF_I2S_DIN_PIN): pins.internal_gpio_input_pin_number,
        cv.Optional(CONF_CHANNEL, default="right"): cv.enum(CHANNELS),
        cv.Optional(CONF_SAMPLE_RATE, default=16000): cv.int_range(min=1),
        cv.Optional(CONF_PDM, default=False): cv.boolean,
        cv.Optional(CONF_BITS_PER_SAMPLE, default="16bit"): cv.All(
            _validate_bits, cv.enum(BITS_PER_SAMPLE)
        ),
    }
)

CONFIG_SCHEMA_OUT = i2s.CONFIG_SCHEMA_I2S_OUT.extend(
    {
        cv.GenerateID(): cv.declare_id(ADFElementI2SOut),
    }
)

CONFIG_SCHEMA_AW88298 = CONFIG_SCHEMA_OUT.extend(
    cv.Schema({cv.GenerateID(): cv.declare_id(ADFI2SOut_AW88298)}).extend(
        i2c.i2c_device_schema(0x36)
    )
)

CONFIG_SCHEMA_ES7210 = CONFIG_SCHEMA_IN.extend(
    cv.Schema({cv.GenerateID(): cv.declare_id(ADFI2SIn_ES7210)}).extend(
        i2c.i2c_device_schema(0x36)
    )
)


CONFIG_SCHEMA = cv.typed_schema(
    {
        "i2s_tx": CONFIG_SCHEMA_OUT,
        "i2s_rx": CONFIG_SCHEMA_IN,
        "aw88298": CONFIG_SCHEMA_AW88298,
        "es7210": CONFIG_SCHEMA_ES7210,
    },
    lower=True,
    space="-",
    default_type="i2s_tx",
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    await cg.register_parented(var, config[CONF_I2S_AUDIO_ID])
    if config["type"] in ["i2s_tx", "aw88298"]:
        cg.add(var.set_dout_pin(config[CONF_I2S_DOUT_PIN]))
        await i2s.register_dac(var, config[i2s.CONF_I2S_DAC])
    elif config["type"] in ["i2s_rx", "es7210"]:
        cg.add(var.set_din_pin(config[CONF_I2S_DIN_PIN]))
        cg.add(var.set_channel(config[CONF_CHANNEL]))
        cg.add(var.set_sample_rate(config[CONF_SAMPLE_RATE]))
        cg.add(var.set_bits_per_sample(config[CONF_BITS_PER_SAMPLE]))
        cg.add(var.set_pdm(config[CONF_PDM]))
        await i2s.register_adc(var, config[i2s.CONF_I2S_ADC])

    if config["type"] in ["aw88298", "es7210"]:
        cg.add_define("ADF_PIPELINE_I2C_IC")
        await i2c.register_i2c_device(var, config)
