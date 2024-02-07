#pragma once

#ifdef USE_ESP_IDF 
#include "esphome/core/component.h"
#include "esphome/core/log.h"


#include <audio_element.h>
#include <audio_pipeline.h>
#include <vector>

namespace esphome {
namespace esp_adf {


class AudioPipeline;
class ADFPipeline;
class AudioPipelineElement;

enum AudioPipelineElementType: uint8_t{
  AUDIO_PIPELINE_SOURCE = 0,
  AUDIO_PIPELINE_SINK,
  AUDIO_PIPELINE_TRANSFORM
};

class AudioPipelineSettingsRequest {
public:
  AudioPipelineSettingsRequest() = default;
  AudioPipelineSettingsRequest(AudioPipelineElement* sender) : requested_by(sender) {}
  int   sampling_rate{-1};
  int   bit_depth{-1};
  int   number_of_channels{-1};
  float target_volume{-1};
  int   mute{-1};

  bool failed{false};
  int  error_code{0};
  AudioPipelineElement* requested_by{nullptr};
  AudioPipelineElement* failed_by{nullptr};
};


class AudioPipelineElement {
public:
  virtual ~AudioPipelineElement(){}
  virtual AudioPipelineElementType get_element_type() const = 0;
  virtual const std::string get_name() = 0;  
  
  virtual void on_pipeline_status_change(){}
  virtual void on_settings_request(AudioPipelineSettingsRequest &request){}
};


/*
Represents and manages one or more ADF-SDK audio elements which form a logical unit.
e.g. HttpStreamer and Decoder, re-sampler and stream_writer  
*/
class ADFPipelineElement : public AudioPipelineElement {
public:
  std::vector<audio_element_handle_t> get_adf_elements(){ return sdk_audio_elements_; }
  std::string get_adf_element_tag(int element_indx );
  void init_adf_elements() {init_adf_elements_(); }
  void deinit_adf_elements(){}
  
  void set_pipeline(ADFPipeline* pipeline){pipeline_ = pipeline;}
protected:
  friend  class ADFPipeline;
  
  virtual void init_adf_elements_() = 0;
  virtual void deinit_adf_elements_();
  virtual void sdk_event_handler_(audio_event_iface_msg_t &msg){}

  std::vector<audio_element_handle_t> sdk_audio_elements_;
  std::vector<std::string> sdk_element_tags_;
  ADFPipeline* pipeline_{nullptr};
};


}
}
#endif