#pragma once

#ifdef USE_ESP_IDF
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include "esphome/components/adf_pipeline/adf_audio_sinks.h"
#include "../usb_audio.h"

namespace esphome {
using namespace esp_adf;
namespace usb_audio {

class USBStreamWriter : public USBAudioComponent, public ADFPipelineSinkElement, public Component {
public:
const std::string get_name() override {return "USB-Audio-Out";}
bool is_ready() override;
bool preparing_step();
//bool pause_elements(bool initial_call) override;
//bool resume_elements() override;

protected:
bool init_adf_elements_() override;
void clear_adf_elements_() override;
void reset_() override;

void on_settings_request(AudioPipelineSettingsRequest &request) override;

audio_element_handle_t usb_audio_stream_{nullptr};
bool usb_stream_started_{false};
};


}
}


#endif
