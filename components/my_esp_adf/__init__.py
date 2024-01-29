from esphome.const import (
    CONF_ID,
    KEY_CORE,
    KEY_FRAMEWORK_VERSION,
)

import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.core import CORE, coroutine_with_priority
from esphome.components.esp32 import add_idf_component
from esphome.components import esp32

CODEOWNERS = ["@gnumpi"]
DEPENDENCIES = []

IS_PLATFORM_COMPONENT = True

CONF_ADF_COMPONENT_TYPE = "type"
CONF_ADF_PIPELINE = "pipeline"
CONF_ADF_COMP_ID = "adf_comp_id"

esp_adf_ns = cg.esphome_ns.namespace("esp_adf")
ADFAudioComponent = esp_adf_ns.class_("ADFAudioComponent")


async def setup_adf_component_core_(var, config):
    pass


async def register_adf_component(var, config):
    if not CORE.has_id(config[CONF_ID]):
        var = cg.Pvariable(config[CONF_ID], var)
    await setup_adf_component_core_(var, config)

ADF_COMPONENT_SCHEMA = cv.Schema({})



@coroutine_with_priority(55.0)
async def to_code(config):
    
    cg.add_define("USE_ESP_ADF")
    cg.add_platformio_option("build_unflags", "-Wl,--end-group")

    cg.add_platformio_option(
        "board_build.embed_txtfiles", "components/dueros_service/duer_profile"
    )

    #if CORE.using_esp_idf and CORE.data[KEY_CORE][KEY_FRAMEWORK_VERSION] >= cv.Version(
    #    6, 0, 0
    #):
    if True:
        add_idf_component(
            name="mdns",
            repo="https://github.com/espressif/esp-adf.git",
            ref="v2.5",
            path="components",
            submodules=["components/esp-adf-libs","components/esp-sr"],
            components=["*"]
        )

        esp32.add_idf_sdkconfig_option("CONFIG_ESP_TLS_INSECURE", True)
        esp32.add_idf_sdkconfig_option("CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY", True)