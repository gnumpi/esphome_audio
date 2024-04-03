import esphome.config_validation as cv
import esphome.final_validate as fv
import esphome.codegen as cg

from esphome import pins
from esphome.components import i2c
from esphome.const import CONF_ENABLE_PIN, CONF_ID, CONF_MODEL
from esphome.components.esp32 import get_esp32_variant
from esphome.components.esp32.const import (
    VARIANT_ESP32,
    VARIANT_ESP32S2,
    VARIANT_ESP32S3,
    VARIANT_ESP32C3,
)

CODEOWNERS = ["@jesserockz"]
DEPENDENCIES = ["esp32"]
MULTI_CONF = True

CONF_I2S_DOUT_PIN = "i2s_dout_pin"
CONF_I2S_DIN_PIN = "i2s_din_pin"
CONF_I2S_MCLK_PIN = "i2s_mclk_pin"
CONF_I2S_BCLK_PIN = "i2s_bclk_pin"
CONF_I2S_LRCLK_PIN = "i2s_lrclk_pin"

CONF_I2S_AUDIO = "i2s_audio"
CONF_I2S_AUDIO_ID = "i2s_audio_id"

CONF_SAMPLE_RATE = "sample_rate"
CONF_I2S_DAC = "dac"
CONF_I2S_ADC = "adc"

i2s_audio_ns = cg.esphome_ns.namespace("i2s_audio")
I2SAudioComponent = i2s_audio_ns.class_("I2SAudioComponent", cg.Component)
I2SAudioIn = i2s_audio_ns.class_("I2SAudioIn", cg.Parented.template(I2SAudioComponent))
I2SAudioOut = i2s_audio_ns.class_(
    "I2SAudioOut", cg.Parented.template(I2SAudioComponent)
)

# https://github.com/espressif/esp-idf/blob/master/components/soc/{variant}/include/soc/soc_caps.h
I2S_PORTS = {
    VARIANT_ESP32: 2,
    VARIANT_ESP32S2: 1,
    VARIANT_ESP32S3: 2,
    VARIANT_ESP32C3: 1,
}

i2s_channel_fmt_t = cg.global_ns.enum("i2s_channel_fmt_t")
CHANNEL_FORMAT = {
    # Only load data in left channel (mono mode)
    "left": i2s_channel_fmt_t.I2S_CHANNEL_FMT_ONLY_LEFT,
    # Only load data in right channel (mono mode)
    "right": i2s_channel_fmt_t.I2S_CHANNEL_FMT_ONLY_RIGHT,
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


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(I2SAudioComponent),
        cv.Required(CONF_I2S_LRCLK_PIN): pins.internal_gpio_output_pin_number,
        cv.Optional(CONF_I2S_BCLK_PIN): pins.internal_gpio_output_pin_number,
        cv.Optional(CONF_I2S_MCLK_PIN): pins.internal_gpio_output_pin_number,
        cv.Optional(CONF_SAMPLE_RATE, default=16000): cv.int_range(min=1),
    }
)


def _final_validate(_):
    i2s_audio_configs = fv.full_config.get()[CONF_I2S_AUDIO]
    variant = get_esp32_variant()
    if variant not in I2S_PORTS:
        raise cv.Invalid(f"Unsupported variant {variant}")
    if len(i2s_audio_configs) > I2S_PORTS[variant]:
        raise cv.Invalid(
            f"Only {I2S_PORTS[variant]} I2S audio ports are supported on {variant}"
        )


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_lrclk_pin(config[CONF_I2S_LRCLK_PIN]))
    if CONF_I2S_BCLK_PIN in config:
        cg.add(var.set_bclk_pin(config[CONF_I2S_BCLK_PIN]))
    if CONF_I2S_MCLK_PIN in config:
        cg.add(var.set_mclk_pin(config[CONF_I2S_MCLK_PIN]))


EXTERNAL_ADC_MODELS = ["generic", "es7210"]
EXTERNAL_DAC_MODELS = ["generic", "aw88298", "es8388"]
EXTERNAL_DAC_OPTIONS = ["mono", "stereo"]

CONF_INIT_VOL = "init_volume"

ExternalDAC = i2s_audio_ns.class_("ExternalDAC", i2c.I2CDevice)
AW88298 = i2s_audio_ns.class_("AW88298", ExternalDAC, i2c.I2CDevice)

CONFIG_SCHEMA_EXT_DAC = cv.typed_schema(
    {
        "generic": cv.Schema({}),
        "aw88298": cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(AW88298),
                cv.Optional(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
            }
        ).extend(i2c.i2c_device_schema(0x36)),
    },
    key=CONF_MODEL,
)

ExternalADC = i2s_audio_ns.class_("ExternalADC", i2c.I2CDevice)
ES7210 = i2s_audio_ns.class_("ES7210", ExternalADC, i2c.I2CDevice)

CONFIG_SCHEMA_EXT_ADC = cv.typed_schema(
    {
        "generic": cv.Schema({}),
        "es7210": cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(ES7210),
                cv.Optional(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
            }
        ).extend(i2c.i2c_device_schema(0x36)),
    },
    key=CONF_MODEL,
)

CONFIG_SCHEMA_I2S_OUT = cv.Schema(
    {
        cv.GenerateID(CONF_I2S_AUDIO_ID): cv.use_id(I2SAudioComponent),
        cv.Required(CONF_I2S_DOUT_PIN): pins.internal_gpio_output_pin_number,
        cv.Optional(
            CONF_I2S_DAC, default={CONF_MODEL: "generic"}
        ): CONFIG_SCHEMA_EXT_DAC,
    }
)

CONFIG_SCHEMA_I2S_IN = cv.Schema(
    {
        cv.GenerateID(CONF_I2S_AUDIO_ID): cv.use_id(I2SAudioComponent),
        cv.Required(CONF_I2S_DIN_PIN): pins.internal_gpio_input_pin_number,
        cv.Optional(
            CONF_I2S_ADC, default={CONF_MODEL: "generic"}
        ): CONFIG_SCHEMA_EXT_ADC,
    }
)


async def register_dac(i2scomp, config: dict) -> None:
    if config["model"] in ["aw88298"]:
        cg.add_define("I2S_EXTERNAL_DAC")
        dac = cg.new_Pvariable(config[CONF_ID])
        cg.add(i2scomp.set_external_dac(dac))
        await i2c.register_i2c_device(dac, config)
        if CONF_ENABLE_PIN in config:
            en_pin = await cg.gpio_pin_expression(config[CONF_ENABLE_PIN])
            cg.add(dac.set_gpio_enable(en_pin))


async def register_adc(i2scomp, config: dict) -> None:
    if config["model"] in ["es7210"]:
        cg.add_define("I2S_EXTERNAL_ADC")
        adc = cg.new_Pvariable(config[CONF_ID])
        cg.add(i2scomp.set_external_adc(adc))
        await i2c.register_i2c_device(adc, config)
        if CONF_ENABLE_PIN in config:
            en_pin = await cg.gpio_pin_expression(config[CONF_ENABLE_PIN])
            cg.add(adc.set_gpio_enable(en_pin))
