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


enum class PipelineElementState : uint8_t { UNINITIALIZED = 0, INITIALIZED, PREPARE, PREPARING, WAIT_FOR_PREPARATION_DONE, READY };

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

  virtual void on_pipeline_status_change() {}
  virtual void on_settings_request(AudioPipelineSettingsRequest &request) {}


  std::vector<audio_element_handle_t> get_adf_elements() { return sdk_audio_elements_; }
  std::string get_adf_element_tag(int element_indx);
  bool init_adf_elements() { return init_adf_elements_(); }
  void destroy_adf_elements() {clear_adf_elements_();}
  virtual void prepare_elements() {}

  void set_pipeline(ADFPipeline *pipeline) { pipeline_ = pipeline; }
  virtual bool is_ready() {return true;}
  virtual bool requires_destruction_on_stop(){ return false; }

 protected:
  friend class ADFPipeline;

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
