import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID


from .. import usb_audio_ns
from ... import adf_pipeline as esp_adf

CODEOWNERS = ["@gnumpi"]
DEPENDENCIES = ["adf_pipeline"]

USBStreamWriter = usb_audio_ns.class_(
    "USBStreamWriter", esp_adf.ADFPipelineSink, cg.Component
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(USBStreamWriter),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
