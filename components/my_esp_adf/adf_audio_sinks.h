#pragma once

#ifdef USE_ESP_IDF 

#include "adf_audio_element.h"

namespace esphome {
namespace esp_adf {

class ADFPipelineSinkElement: public ADFPipelineElement {
public:
  const AudioPipelineElementType get_element_type() const {return AudioPipelineElementType::AUDIO_PIPELINE_SINK};
  bool hasInputBuffer()  const override {return false;}
  bool hasOutputBuffer() const override {return true; }
};

}
}
#endif