#pragma once

#ifdef USE_ESP_IDF

#include "adf_audio_element.h"

namespace esphome {
namespace esp_adf {

class ADFPipelineProcessElement : public ADFPipelineElement {
 public:
  AudioPipelineElementType get_element_type() const { return AudioPipelineElementType::AUDIO_PIPELINE_PROCESS; }
};

class ADFResampler : public ADFPipelineProcessElement {
 public:
  const std::string get_name() override { return "Resampler"; }

 protected:
  bool init_adf_elements_() override;
  //void clear_adf_elements_() override;
  void on_settings_request(AudioPipelineSettingsRequest &request) override;

  int src_rate_{16000};
  int dst_rate_{16000};
  int src_num_channels_{2};
  int dst_num_channels_{2};

  audio_element_handle_t sdk_resampler_;
};

}  // namespace esp_adf
}  // namespace esphome
#endif
