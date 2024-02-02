#pragma once

#ifdef USE_ESP_IDF 

#include "adf_audio_element.h"

namespace esphome {
namespace esp_adf {

class ADFPipelineSinkElement: public ADFPipelineElement {
public:
  AudioPipelineElementType get_element_type() const {return AudioPipelineElementType::AUDIO_PIPELINE_SINK;}
};

}
}
#endif