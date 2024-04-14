"""ADF-Pipeline platform implementation of I2S controller (TX and RX)."""

import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.const import CONF_ID

from ... import adf_pipeline as esp_adf

from .. import (
    I2SReader,
    I2SWriter,
    i2s_audio_ns,
    I2S_AUDIO_IN,
    I2S_AUDIO_OUT,
    CONFIG_SCHEMA_I2S_READER,
    CONFIG_SCHEMA_I2S_WRITER,
    #    final_validate_device_schema,
    register_i2s_reader,
    register_i2s_writer,
)

CODEOWNERS = ["@gnumpi"]
AUTO_LOAD = ["adf_pipeline"]
DEPENDENCIES = ["adf_pipeline", "i2s_audio"]


ADFElementI2SOut = i2s_audio_ns.class_(
    "ADFElementI2SOut",
    esp_adf.ADFPipelineSink,
    esp_adf.ADFPipelineElement,
    I2SWriter,
    cg.Component,
)

ADFElementI2SIn = i2s_audio_ns.class_(
    "ADFElementI2SIn",
    esp_adf.ADFPipelineSource,
    esp_adf.ADFPipelineElement,
    I2SReader,
    cg.Component,
)

CONF_USE_ADF_ALC = "adf_alc"

CONFIG_SCHEMA_IN = CONFIG_SCHEMA_I2S_READER.extend(
    {
        cv.GenerateID(): cv.declare_id(ADFElementI2SIn),
    }
)

CONFIG_SCHEMA_OUT = CONFIG_SCHEMA_I2S_WRITER.extend(
    {
        cv.GenerateID(): cv.declare_id(ADFElementI2SOut),
        cv.Optional(CONF_USE_ADF_ALC, default=True): cv.boolean,
    }
)

CONFIG_SCHEMA = cv.typed_schema(
    {
        I2S_AUDIO_IN: CONFIG_SCHEMA_IN,
        I2S_AUDIO_OUT: CONFIG_SCHEMA_OUT,
    },
    lower=True,
    space="-",
)

# FINAL_VALIDATE_SCHEMA = final_validate_device_schema("adf_i2s")


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if config["type"] == I2S_AUDIO_IN:
        await register_i2s_reader(var, config)

    elif config["type"] == I2S_AUDIO_OUT:
        await register_i2s_writer(var, config)
        cg.add(var.set_use_adf_alc(config[CONF_USE_ADF_ALC]))
