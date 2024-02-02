#pragma once

#ifdef USE_ESP_IDF 
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "adf_pipeline.h"

#include <audio_element.h>
#include <audio_pipeline.h>
#include <vector>

namespace esphome {
namespace esp_adf {

class AudioPipelineElement;

enum AudioPipelineElementType: uint8_t{
  AUDIO_PIPELINE_SOURCE = 0,
  AUDIO_PIPELINE_SINK,
  AUDIO_PIPELINE_TRANSFORM
};


enum AudioPipelineEventType: uint8_t {
  AUDIO_PIPELINE_STATUS_CHANGED = 0,
  AUDIO_PIPELINE_SETTINGS_UPDATED,
  AUDIO_PIPELINE_SETTINGS_REQUESTED
};


class AudioPipelineSettingsRequest {
public:
  AudioPipelineSettingsRequest() = default;
  AudioPipelineSettingsRequest(AudioPipelineElement* sender) requested_by(sender) {}
  int  sampling_rate{-1};
  int  bit_depth{-1};
  int  number_of_channels{-1};
  int  target_volume{-1};
  int  mute{-1};

  bool failed{false};
  int error_code{0};
  AudioPipelineElement* requested_by{nullptr};
  AudioPipelineElement* failed_by{nullptr};
};


class AudioPipelineEvent{
  AudioPipelineEventType event_type;
  AudioPipelineElement* sender;

};


class AudioPipelineElement {
public:
  virtual ~AudioPipelineElement(){}
  virtual const AudioPipelineElementType const get_element_type() = 0;
  virtual const std::string get_name() = 0;  
  void set_pipeline(AudioPipeline* pipeline){pipeline_ = pipeline;}

protected:
  virtual void on_pipeline_status_change(AudioPipelineEvent &event){}
  virtual void on_settings_request(AudioPipelineSettingsRequest &request){}
  AudioPipeline* pipeline_{nullptr};
};


/*
Represents and manages one or more esp-adf audio_elements which form a logical unit.
e.g. HttpStreamer and Decoder, re-sampler and stream_writer  
*/
class ADFPipelineElement : public AudioPipelineElement {
public:
  std::vector<audio_element_handle_t> get_adf_elements(){ return this->sdk_audio_elements_; }
  std::string get_adf_element_tag(int element_indx );
  void init_adf_elements() {this->init_adf_elements_(); }
  void deinit_adf_elements(){}
  
  
  virtual bool hasInputBuffer()   const {return false;}
  virtual bool hasOutputBuffer()  const {return false;}
  virtual bool hasVolumeControl() const {return false;}
  virtual bool isMuteable()       const {return false;}

  virtual void mute(){}
  virtual void unmute(){}
  virtual bool is_muted() const {return false;}
  virtual void set_volume(float volume){}
  virtual float get_volume() {return 1.; }

  virtual void set_sampling_frequency(int freq){}
  virtual void set_bit_depth(int bits){}
  virtual void set_number_of_channels(int channels){}

  void adf_reset_states_();
  
protected:
  friend  class ADFPipeline;
  
  virtual void init_adf_elements_() = 0;
  virtual void deinit_adf_elements_();
  virtual void adf_event_handler_(audio_event_iface_msg_t &msg){}

  std::vector<audio_element_handle_t> sdk_audio_elements_;
  std::vector<std::string> sdk_element_tags_;
};


}
}
#endif