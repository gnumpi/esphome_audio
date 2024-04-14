#pragma once

#include <cstring>
#include <vector>
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF

#include <audio_element.h>
#include <audio_pipeline.h>

#include "adf_audio_element.h"

namespace esphome {
namespace esp_adf {


/* Audio Pipeline States:

UNINITIALIZED -> STOPPED -> (PREPARING) -> STARTING -> RUNNING
    -> PAUSING -> PAUSED -> RESUMING -> RUNNING
    -> STOPPING -> STOPPED -> DESTROYING -> UNINITIALIZED

State Explanations:
- UNINITIALIZED: No memory allocated, no tasks created, no hardware blocked.
- STOPPED: Memory allocated, hardware reserved, but no task is running.
- PREPARING: Optional state where dynamic information is received by starting individual components,
             pipeline elements are reconfigured accordingly, and finally the ring buffers are reset if necessary.
- STARTING: Tasks of all pipeline elements are requested to start.
- RUNNING: Last element in the pipeline reported to be running.
- PAUSING: Transition state for pausing operations.
- PAUSED: Pipeline is paused.
- RESUMING: Transition state for resuming operations.
- STOPPING: Transition state for stopping operations.
- DESTROYING: Freeing all memory and hardware reservations.

*/
enum PipelineState : uint8_t { UNINITIALIZED = 0, PREPARING, STARTING, RUNNING, STOPPING, STOPPED, PAUSING, PAUSED, RESUMING, DESTROYING };


class ADFPipelineController;

/* Encapsulates the core functionalities of the ADF pipeline.
This includes constructing the pipeline and managing its lifecycle.
*/
class ADFPipeline {
 public:
  ADFPipeline(ADFPipelineController *parent) { parent_ = parent; }
  virtual ~ADFPipeline() {}

  void start();
  void stop();
  void pause();
  void resume();
  void destroy();

  PipelineState getState() { return state_; }
  void loop() { this->watch_(); }

  void set_destroy_on_stop(bool value){ this->destroy_on_stop_ = value; }
  void append_element(ADFPipelineElement *element);
  int get_number_of_elements() { return pipeline_elements_.size(); }
  std::vector<std::string> get_element_names();
  void dump_element_configs();

  // Send a settings request to all pipeline elements
  bool request_settings(AudioPipelineSettingsRequest &request);
  void on_settings_request_failed(AudioPipelineSettingsRequest request) {}

 protected:
  bool init_();
  bool reset_();
  bool start_();
  bool stop_();
  bool pause_();
  bool resume_();
  bool deinit_();

  void set_state_(PipelineState state);

  void loop_();
  void watch_();
  void check_all_started_();
  void check_all_stopped_();
  void prepare_elements_();
  void check_if_components_are_ready_();
  void check_for_pipeline_events_();
  void forward_event_to_pipeline_elements_(audio_event_iface_msg_t &msg);

  bool build_adf_pipeline_();
  void deinit_all_();

  audio_pipeline_handle_t adf_pipeline_{};
  audio_event_iface_handle_t adf_pipeline_event_{};
  audio_element_handle_t adf_last_element_in_pipeline_{};
  std::vector<ADFPipelineElement *> pipeline_elements_;
  ADFPipelineController *parent_{nullptr};

  PipelineState state_{PipelineState::UNINITIALIZED};
  bool destroy_on_stop_{false};
  uint32_t preparation_started_at_{0};
};

}  // namespace esp_adf
}  // namespace esphome

#endif
