""" """

# import esphome.codegen as cg
import esphome.config_validation as cv

ADFResampler = None

CODEOWNERS = ["@gnumpi"]
DEPENDENCIES = []


CONFIG_SCHEMA_RESAMPLER = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ADFResampler),
    }
)
