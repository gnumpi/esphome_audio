from collections import defaultdict

import esphome.config_validation as cv
import esphome.final_validate as fv
import esphome.codegen as cg

from esphome import pins
from esphome.components import i2c
from esphome.const import (
    CONF_ENABLE_PIN,
    CONF_ID,
    CONF_MODE,
    CONF_MODEL,
)
from esphome.components.esp32 import get_esp32_variant
from esphome.components.esp32.const import (
    VARIANT_ESP32,
    VARIANT_ESP32S2,
    VARIANT_ESP32S3,
    VARIANT_ESP32C3,
)

from . import i2s_settings as i2s

CODEOWNERS = ["@jesserockz", "@gnumpi"]
DEPENDENCIES = ["esp32"]
MULTI_CONF = True

CONF_I2S_DOUT_PIN = "i2s_dout_pin"
CONF_I2S_DIN_PIN = "i2s_din_pin"
CONF_I2S_MCLK_PIN = "i2s_mclk_pin"
CONF_I2S_BCLK_PIN = "i2s_bclk_pin"
CONF_I2S_LRCLK_PIN = "i2s_lrclk_pin"

CONF_I2S_AUDIO = "i2s_audio"
CONF_I2S_AUDIO_ID = "i2s_audio_id"
CONF_I2S_ACCESS_MODE = "access_mode"

CONF_SAMPLE_RATE = "sample_rate"
CONF_BITS_PER_SAMPLE = "bits_per_sample"
CONF_PDM = "pdm"

CONF_ADC_TYPE = "adc_type"
CONF_DAC_TYPE = "dac_type"
CONF_I2S_DAC = "dac"
CONF_I2S_ADC = "adc"

i2s_audio_ns = cg.esphome_ns.namespace("i2s_audio")
I2SAudioComponent = i2s_audio_ns.class_("I2SAudioComponent", cg.Component)

# https://github.com/espressif/esp-idf/blob/master/components/soc/{variant}/include/soc/soc_caps.h
I2S_PORTS = {
    VARIANT_ESP32: 2,
    VARIANT_ESP32S2: 1,
    VARIANT_ESP32S3: 2,
    VARIANT_ESP32C3: 1,
}

I2SAccessMode = i2s_audio_ns.enum("I2SAccessMode", is_class=True)
ACCESS_MODES = {"exclusive": I2SAccessMode.EXCLUSIVE, "duplex": I2SAccessMode.DUPLEX}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(I2SAudioComponent),
        cv.Required(CONF_I2S_LRCLK_PIN): pins.internal_gpio_output_pin_number,
        cv.Optional(CONF_I2S_BCLK_PIN): pins.internal_gpio_output_pin_number,
        cv.Optional(CONF_I2S_MCLK_PIN): pins.internal_gpio_output_pin_number,
        cv.Optional(CONF_I2S_ACCESS_MODE, default="exclusive"): cv.enum(ACCESS_MODES),
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
    cg.add(var.set_access_mode(config[CONF_I2S_ACCESS_MODE]))
    cg.add(var.set_lrclk_pin(config[CONF_I2S_LRCLK_PIN]))
    if CONF_I2S_BCLK_PIN in config:
        cg.add(var.set_bclk_pin(config[CONF_I2S_BCLK_PIN]))
    if CONF_I2S_MCLK_PIN in config:
        cg.add(var.set_mclk_pin(config[CONF_I2S_MCLK_PIN]))


I2SReader = i2s_audio_ns.class_("I2SReader", cg.Parented.template(I2SAudioComponent))
I2SWriter = i2s_audio_ns.class_("I2SWriter", cg.Parented.template(I2SAudioComponent))

ExternalDAC = i2s_audio_ns.class_("ExternalDAC", i2c.I2CDevice)
AW88298 = i2s_audio_ns.class_("AW88298", ExternalDAC, i2c.I2CDevice)
ES8388 = i2s_audio_ns.class_("ES8388", ExternalDAC, i2c.I2CDevice)

ExternalADC = i2s_audio_ns.class_("ExternalADC", i2c.I2CDevice)
ES7210 = i2s_audio_ns.class_("ES7210", ExternalADC, i2c.I2CDevice)


I2S_AUDIO_IN = "audio_in"
I2S_AUDIO_OUT = "audio_out"


CONFIG_SCHEMA_DAC = cv.typed_schema(
    {
        "generic": cv.Schema({}),
        "internal": cv.Schema(
            {
                cv.GenerateID(CONF_I2S_AUDIO_ID): cv.use_id(I2SAudioComponent),
                cv.Required(CONF_MODE): cv.enum(i2s.INTERNAL_DAC_OPTIONS, lower=True),
            }
        ),
        "aw88298": cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(AW88298),
                cv.Optional(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
            }
        ).extend(i2c.i2c_device_schema(0x36)),
        "es8388": cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(ES8388),
            }
        ).extend(i2c.i2c_device_schema(0x10)),
    },
    key=CONF_MODEL,
    default_type="generic",
)

CONFIG_SCHEMA_I2S_WRITER = i2s.CONFIG_SCHEMA_I2S_COMMON.extend(
    {
        cv.GenerateID(CONF_I2S_AUDIO_ID): cv.use_id(I2SAudioComponent),
        cv.Required(CONF_I2S_DOUT_PIN): pins.internal_gpio_output_pin_number,
        cv.Optional(CONF_I2S_DAC, default={CONF_MODEL: "generic"}): CONFIG_SCHEMA_DAC,
    }
).extend(cv.COMPONENT_SCHEMA)


CONFIG_SCHEMA_ADC = cv.typed_schema(
    {
        "generic": cv.Schema({}),
        "internal": cv.Schema({}),
        "es7210": cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(ES7210),
                cv.Optional(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
            }
        ).extend(i2c.i2c_device_schema(0x36)),
    },
    key=CONF_MODEL,
)

CONFIG_SCHEMA_I2S_READER = i2s.CONFIG_SCHEMA_I2S_COMMON.extend(
    {
        cv.GenerateID(CONF_I2S_AUDIO_ID): cv.use_id(I2SAudioComponent),
        cv.Required(CONF_I2S_DIN_PIN): pins.internal_gpio_input_pin_number,
        cv.Required(CONF_PDM): cv.boolean,
        cv.Optional(CONF_I2S_ADC, default={CONF_MODEL: "generic"}): CONFIG_SCHEMA_ADC,
    }
)


def final_validate_device_schema(name: str) -> cv.Schema:
    fv_data_schema = {
        I2S_AUDIO_IN: None,
        I2S_AUDIO_OUT: None,
        CONF_SAMPLE_RATE: None,
        CONF_BITS_PER_SAMPLE: None,
        CONF_PDM: None,
    }

    def parent_validator(config):
        direction = config.get("type")

        def validator(value):
            fconf = fv.full_config.get()
            if CONF_I2S_AUDIO not in fconf.data:
                fconf.data[CONF_I2S_AUDIO] = defaultdict(lambda: fv_data_schema)
            i2s_data = fconf.data[CONF_I2S_AUDIO][value]
            if not i2s_data[direction] is None:
                raise cv.Invalid(
                    f"Multiple I2S devices [{i2s_data[direction]},{name}] registered as {direction} for {value}."
                )
            i2s_data[direction] = name

        return validator

    def create_schema(config):
        return cv.Schema(
            {cv.Required(CONF_I2S_AUDIO_ID): parent_validator(config)},
            extra=cv.ALLOW_EXTRA,
        )(config)

    return create_schema


async def apply_i2s_settings(var, config) -> None:
    cg.add(var.set_channel(config[i2s.CONF_CHANNEL]))
    cg.add(var.set_sample_rate(config[i2s.CONF_SAMPLE_RATE]))
    cg.add(var.set_bits_per_sample(config[i2s.CONF_BITS_PER_SAMPLE]))
    cg.add(var.set_use_apll(config[i2s.CONF_USE_APLL]))
    cg.add(var.set_fixed_settings(config[i2s.CONF_FIXED_SETTINGS]))


async def register_i2s_writer(writer, config: dict) -> None:
    i2s_cntrl = await cg.get_variable(config[CONF_I2S_AUDIO_ID])
    await cg.register_parented(writer, config[CONF_I2S_AUDIO_ID])
    cg.add(i2s_cntrl.set_audio_out(writer))
    await apply_i2s_settings(writer, config)

    if CONF_I2S_DOUT_PIN in config:
        cg.add(writer.set_dout_pin(config[CONF_I2S_DOUT_PIN]))

    if CONF_I2S_DAC in config:
        dac_cfg = config[CONF_I2S_DAC]
        if dac_cfg["model"] in ["aw88298", "es8388"]:
            cg.add_define("I2S_EXTERNAL_DAC")
            dac = cg.new_Pvariable(dac_cfg[CONF_ID])
            cg.add(writer.set_external_dac(dac))
            await i2c.register_i2c_device(dac, dac_cfg)

            if CONF_ENABLE_PIN in config:
                en_pin = await cg.gpio_pin_expression(dac_cfg[CONF_ENABLE_PIN])
                cg.add(dac.set_gpio_enable(en_pin))


async def register_i2s_reader(reader, config: dict) -> None:
    i2s_cntrl = await cg.get_variable(config[CONF_I2S_AUDIO_ID])
    await cg.register_parented(reader, config[CONF_I2S_AUDIO_ID])
    cg.add(i2s_cntrl.set_audio_in(reader))

    await apply_i2s_settings(reader, config)
    cg.add(reader.set_pdm(config[CONF_PDM]))

    if CONF_I2S_DIN_PIN in config:
        cg.add(reader.set_din_pin(config[CONF_I2S_DIN_PIN]))

    if CONF_I2S_ADC in config:
        adc_cfg = config[CONF_I2S_ADC]
        if adc_cfg["model"] in ["es7210"]:
            cg.add_define("I2S_EXTERNAL_ADC")
            adc = cg.new_Pvariable(adc_cfg[CONF_ID])
            cg.add(reader.set_external_adc(adc))
            await i2c.register_i2c_device(adc, adc_cfg)
            if CONF_ENABLE_PIN in adc_cfg:
                en_pin = await cg.gpio_pin_expression(adc_cfg[CONF_ENABLE_PIN])
                cg.add(adc.set_gpio_enable(en_pin))
