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


COMPONENT_TYPES = ["sink", "source", "filter"]
SELF_DESCRIPTORS = ["this","source","sink","self"]
ADF_COMPONENT_SCHEMA = cv.Schema({
    cv.Optional(CONF_ADF_COMPONENT_TYPE) : cv.one_of(*COMPONENT_TYPES),
    cv.Optional(CONF_ADF_PIPELINE) : cv.ensure_list( 
        cv.Any( 
            cv.one_of(*SELF_DESCRIPTORS),
            cv.use_id(ADFAudioComponent)
        )
    )
})



async def add_pipeline_elements(var, config):
    if CONF_ADF_COMP_ID in config:
        comp = await cg.get_variable(config[CONF_ADF_COMP_ID])
        cg.add( var.add_element_to_pipeline(comp) )
    
    if CONF_ADF_PIPELINE in config:
        for comp_id in config[CONF_ADF_PIPELINE]:
            if comp_id not in SELF_DESCRIPTORS:
                comp = await cg.get_variable(comp_id)
                cg.add( var.add_element_to_pipeline(comp) )
            else:
                cg.add( var.append_own_elements() )



@coroutine_with_priority(55.0)
async def to_code(config):
    
    cg.add_define("USE_ESP_ADF")
    cg.add_platformio_option("build_unflags", "-Wl,--end-group")

    cg.add_platformio_option(
        "board_build.embed_txtfiles", "components/dueros_service/duer_profile"
    )

    esp32.add_idf_sdkconfig_option("CONFIG_ESP_TLS_INSECURE", True)
    esp32.add_idf_sdkconfig_option("CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY", True)
    
    add_idf_component(
        name="mdns",
        repo="https://github.com/espressif/esp-adf.git",
        ref="v2.5",
        path="components",
        submodules=["components/esp-adf-libs","components/esp-sr"],
        components=["*"]
    )

    