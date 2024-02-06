#pragma once

#ifdef USE_ESP_IDF

#include "../i2s_audio.h"

#include "esphome/core/component.h"

#include "../../my_esp_adf/adf_audio_sinks.h"

namespace esphome {
using namespace esp_adf;
namespace i2s_audio {

class ADFElementI2SOut : public I2SAudioOut, public ADFPipelineSinkElement, public Component {
public:
    //ESPHome Component implementations
    void setup() override;
    
    // ADFPipelieSourceElement implementations
    const std::string get_name() override {return "I2S_Input";}
    
    void set_dout_pin(uint8_t pin) { this->dout_pin_ = pin; }
    void set_external_dac_channels(uint8_t channels) { this->external_dac_channels_ = channels; }

protected:
    void on_settings_request(AudioPipelineSettingsRequest &request) override;
    uint8_t dout_pin_{0};
    uint8_t external_dac_channels_;

    uint32_t sample_rate_;
    uint8_t  bits_per_sample_;

    std::vector<uint32_t> supported_samples_rates_;
    std::vector<uint8_t>  supported_bits_per_sample_;
    
    void init_adf_elements_() override; 
    audio_element_handle_t adf_i2s_stream_writer_;
};


}
}
#endif
