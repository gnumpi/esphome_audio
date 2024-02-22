#pragma once

#ifdef USE_ESP_IDF

#include "adf_audio_element.h"
#include "filter_resample.h"

namespace esphome {
namespace esp_adf {

class ADFPipelineProcessElement : public ADFPipelineElement {
 public:
  AudioPipelineElementType get_element_type() const { return AudioPipelineElementType::AUDIO_PIPELINE_TRANSFORM; }
};

class ADFResampler : public ADFPipelineProcessElement {
public:
  const std::string get_name() override { return "Resampler"; }

protected:
  void init_adf_elements_() override;
  bool validate_settings_();

  esp_resample_mode_t mode_;
  pcm_format format_in_;
  pcm_format format_out_;
  int down_ch_idx_;

  audio_element_handle_t adf_resample_filter_;
};


}
}

#endif
