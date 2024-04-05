#pragma once

#ifdef USE_ESP_IDF

#include "../i2s_audio.h"

#include "esphome/core/component.h"

#include "../../adf_pipeline/adf_audio_sources.h"

namespace esphome {
using namespace esp_adf;
namespace i2s_audio {

class ADFElementI2SIn : public I2SAudioIn, public ADFPipelineSourceElement, public Component {
 public:
  // ESPHome Component implementations
  void setup() override {}

  // ADFPipelieSourceElement implementations
  const std::string get_name() override { return "I2S_Input"; }
  bool is_ready() override;

  //void set_din_pin(int8_t pin) { this->din_pin_ = pin; }
  void set_pdm(bool pdm) { this->pdm_ = pdm; }
  void set_channel(i2s_channel_fmt_t channel) { this->channel_ = channel; }
  void set_sample_rate(uint32_t sample_rate) { this->sample_rate_ = sample_rate; }
  void set_bits_per_sample(i2s_bits_per_sample_t bits_per_sample) { this->bits_per_sample_ = bits_per_sample; }
  void set_use_apll(uint32_t use_apll) { this->use_apll_ = use_apll; }

 protected:
  bool pdm_{false};
  //int8_t din_pin_{I2S_PIN_NO_CHANGE};
  i2s_channel_fmt_t channel_{I2S_CHANNEL_FMT_RIGHT_LEFT};
  uint32_t sample_rate_{16000};
  i2s_bits_per_sample_t bits_per_sample_{I2S_BITS_PER_SAMPLE_16BIT};
  bool use_apll_;
  bool valid_settings_{false};
  bool init_adf_elements_() override;
  void clear_adf_elements_() override;
  audio_element_handle_t adf_i2s_stream_reader_;
};

}  // namespace i2s_audio
}  // namespace esphome
#endif
