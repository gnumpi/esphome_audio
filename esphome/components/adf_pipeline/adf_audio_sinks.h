#pragma once

#ifdef USE_ESP_IDF

#include "adf_audio_element.h"

namespace esphome {
namespace esp_adf {

class ADFPipelineSinkElement : public ADFPipelineElement {
 public:
  AudioPipelineElementType get_element_type() const { return AudioPipelineElementType::AUDIO_PIPELINE_SINK; }
};

class PCMSink : public ADFPipelineSinkElement {
 public:
  const std::string get_name() override { return "PCMSink"; }
  int stream_read_bytes(char *buffer, int len);
  uint8_t get_bits_per_sample() { return this->bits_per_sample_; }

 protected:
  bool init_adf_elements_() override;
  void clear_adf_elements_() override;
  void on_settings_request(AudioPipelineSettingsRequest &request) override;

  uint8_t bits_per_sample_{16};
  audio_element_handle_t adf_raw_stream_reader_;
};

}  // namespace esp_adf
}  // namespace esphome
#endif
