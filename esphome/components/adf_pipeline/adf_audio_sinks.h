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
  int stream_read(char *buffer, int len);

 protected:
  bool init_adf_elements_() override;
  void clear_adf_elements_() override;
  audio_element_handle_t adf_raw_stream_reader_;
};

}  // namespace esp_adf
}  // namespace esphome
#endif
