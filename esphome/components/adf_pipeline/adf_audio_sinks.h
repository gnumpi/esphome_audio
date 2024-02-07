#pragma once

#ifdef USE_ESP_IDF 

#include "adf_audio_element.h"

namespace esphome {
namespace esp_adf {


class ADFPipelineSinkElement: public ADFPipelineElement {
public:
  AudioPipelineElementType get_element_type() const {return AudioPipelineElementType::AUDIO_PIPELINE_SINK;}
};


class I2SWriter : public ADFPipelineSinkElement {
public:
  const std::string get_name() override {return "I2SWriter";} 
protected:
  void init_adf_elements_() override {}
  audio_element_handle_t adf_i2s_stream_writer_;
};


class PCMSink : public ADFPipelineSinkElement {
public:
  const std::string get_name() override {return "PCMSink";}
  int stream_read(char* buffer, int len);
protected:
  void init_adf_elements_() override; 
  audio_element_handle_t adf_raw_stream_reader_;

};


}
}
#endif