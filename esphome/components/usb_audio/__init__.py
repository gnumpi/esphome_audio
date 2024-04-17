import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import esp32

CODEOWNERS = ["@gnumpi"]
DEPENDENCIES = ["esp32"]

usb_audio_ns = cg.esphome_ns.namespace("usb_audio")

CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    cg.add_define("USE_ESP32_USB_STREAM")

    esp32.add_idf_component(
        name="usb_stream",
        repo="https://github.com/espressif/esp-iot-solution.git",
        path="components/usb/usb_stream",
    )

    esp32.add_idf_sdkconfig_option("CONFIG_USB_OTG_SUPPORTED", True)
    esp32.add_idf_sdkconfig_option("CONFIG_SOC_USB_OTG_SUPPORTED", True)
    esp32.add_idf_sdkconfig_option("CONFIG_USB_PROC_TASK_CORE", 0)
    esp32.add_idf_sdkconfig_option("CONFIG_USB_PROC_TASK_PRIORITY", 2)
    esp32.add_idf_sdkconfig_option("CONFIG_NUM_ISOC_SPK_URBS", 6)
