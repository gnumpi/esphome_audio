#pragma once

#ifdef USE_ESP_IDF

#include "../i2s_audio.h"

#include "esphome/core/component.h"

#include "../../adf_pipeline/adf_audio_sinks.h"

namespace esphome {
using namespace esp_adf;
namespace i2s_audio {

class ADFElementI2SOut : public I2SWriter, public ADFPipelineSinkElement, public Component {
 public:
  // ESPHome Component implementations
  void setup() override;

  // ADFPipelieSourceElement implementations
  const std::string get_name() override { return "I2S_Writer"; }
  void dump_config() override { this->dump_i2s_settings(); }
  bool is_ready() override;


  void set_external_dac_channels(uint8_t channels) { this->external_dac_channels_ = channels; }

 protected:
  void on_settings_request(AudioPipelineSettingsRequest &request) override;
  uint8_t external_dac_channels_;
  uint8_t mono_channel_select_;
  uint8_t channels_;
  bool use_adf_alc_{false};
  bool adjustable_{false};

  bool init_adf_elements_() override;
  void clear_adf_elements_() override;
  audio_element_handle_t adf_i2s_stream_writer_;
};

}  // namespace i2s_audio
}  // namespace esphome
#endif
