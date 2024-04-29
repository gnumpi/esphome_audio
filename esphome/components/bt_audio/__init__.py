import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import esp32

CODEOWNERS = ["@gnumpi"]
DEPENDENCIES = ["esp32"]

bt_audio_ns = cg.esphome_ns.namespace("bt_audio")

CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    cg.add_define("USE_ESP32_BT")

    esp32.add_idf_sdkconfig_option("CONFIG_BT_ENABLED", True)
    esp32.add_idf_sdkconfig_option("CONFIG_BT_BLUEDROID_ENABLED", True)
    esp32.add_idf_sdkconfig_option("SMP_ENABLE", True)
    esp32.add_idf_sdkconfig_option("CONFIG_BT_CLASSIC_ENABLED", True)
