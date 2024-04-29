import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID


from .. import bt_audio_ns
from ... import adf_pipeline as esp_adf

CODEOWNERS = ["@gnumpi"]
DEPENDENCIES = ["adf_pipeline"]

AdfBtPiepelineController = bt_audio_ns.class_(
    "AdfBtPiepelineController", esp_adf.ADFPipelineController, cg.Component
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(AdfBtPiepelineController),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
