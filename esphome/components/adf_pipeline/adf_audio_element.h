#pragma once

#ifdef USE_ESP_IDF
#include "esphome/core/log.h"

#include <audio_element.h>
#include <audio_pipeline.h>
#include <vector>

namespace esphome {
namespace esp_adf {

typedef struct {
  int rate;
  int bits;
  int channels;
} pcm_format;


enum class PipelineElementState : uint8_t { UNINITIALIZED = 0, INITIALIZED, PREPARE, PREPARING, WAITING_FOR_SDK_EVENT, READY, PAUSING, RESUMING, RUNNING, PAUSED, STOPPING, STOPPED, ERROR };

class ADFPipeline;
class ADFPipelineElement;

enum AudioPipelineElementType : uint8_t { AUDIO_PIPELINE_SOURCE = 0, AUDIO_PIPELINE_SINK, AUDIO_PIPELINE_PROCESS };


class AudioPipelineSettingsRequest {
 public:
  AudioPipelineSettingsRequest() = default;
  AudioPipelineSettingsRequest(ADFPipelineElement *sender) : requested_by(sender) {}
  int sampling_rate{-1};
  int bit_depth{-1};
  int number_of_channels{-1};
  float target_volume{-1.};
  int mute{-1};

  int final_sampling_rate{-1};
  int final_bit_depth{-1};
  int final_number_of_channels{-1};
  float final_volume{-1.};

  int finish_on_timeout{0};

  bool failed{false};
  int error_code{0};
  ADFPipelineElement *requested_by{nullptr};
  ADFPipelineElement *failed_by{nullptr};
};

/*
Represents and manages one or more ADF-SDK audio elements which form a logical unit.
e.g. HttpStreamer and Decoder, re-sampler and stream_writer
*/
class ADFPipelineElement {
 public:
  virtual ~ADFPipelineElement() {}

  virtual AudioPipelineElementType get_element_type() const = 0;
  virtual const std::string get_name() = 0;
  virtual void dump_config() const {}

  void set_pipeline(ADFPipeline *pipeline) { pipeline_ = pipeline; }
  virtual void on_pipeline_status_change() {}
  virtual void on_settings_request(AudioPipelineSettingsRequest &request) {}

  bool init_adf_elements() { return init_adf_elements_(); }
  void destroy_adf_elements() {clear_adf_elements_();}

  bool in_error_state(){ return this->element_state_ == PipelineElementState::ERROR; }

  bool all_prepared_{false};
  bool all_paused_{false};
  bool all_resumed_{false};
  bool all_stopped_{false};

  virtual bool prepare_elements(bool initial_call);
  virtual bool pause_elements(bool initial_call);
  virtual bool resume_elements(bool initial_call);
  virtual bool stop_elements(bool initial_call);

  virtual bool elements_have_stopped();

  std::vector<audio_element_handle_t> get_adf_elements() { return sdk_audio_elements_; }
  std::string get_adf_element_tag(int element_indx);

  virtual bool is_ready() { return true; }
  virtual bool requires_destruction_on_stop(){ return false; }

  PipelineElementState get_state(){ return this->element_state_; }
  void set_state(PipelineElementState new_state){ this->element_state_ = new_state; }

 protected:
  friend class ADFPipeline;

  PipelineElementState element_state_{PipelineElementState::UNINITIALIZED};

  virtual bool init_adf_elements_() = 0;
  virtual void clear_adf_elements_();
  virtual void reset_() {}
  virtual void sdk_event_handler_(audio_event_iface_msg_t &msg) {}

  std::vector<audio_element_handle_t> sdk_audio_elements_;
  std::vector<std::string> sdk_element_tags_;
  ADFPipeline *pipeline_{nullptr};
};

}  // namespace esp_adf
}  // namespace esphome
#endif
