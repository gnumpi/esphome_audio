from esphome.const import (
    CONF_ID,
    KEY_CORE,
    KEY_FRAMEWORK_VERSION,
)

import esphome.config_validation as cv
from esphome.core import CORE, coroutine_with_priority
from esphome.components.esp32 import add_idf_component

CODEOWNERS = ["@gnumpi"]
DEPENDENCIES = []



@coroutine_with_priority(55.0)
async def to_code(config):

    if CORE.using_esp_idf and CORE.data[KEY_CORE][KEY_FRAMEWORK_VERSION] >= cv.Version(
        6, 0, 0
    ):
        add_idf_component(
            name="mdns",
            repo="https://github.com/espressif/esp-adf.git",
            ref="v2.6",
            path="components",
            submodules=["components/esp-adf-libs","components/esp-sr"],
            components=["esp-adf-libs","audio_pipeline","audio_stream"]
        )
