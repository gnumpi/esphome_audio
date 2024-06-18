#pragma once

#include <cstring>
#include <vector>
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF
#include "esphome/core/hal.h"
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
enum PipelineState : uint8_t {
  UNINITIALIZED = 0,
  INITIALIZING,
  CREATED,
  PREPARING,
  STOPPED,
  STARTING,
  RUNNING,
  FINISHING,
  PAUSING,
  PAUSED,
  ABORTING,
  DESTROYING
};

enum class PipelineRequest : uint8_t { RUNNING, STOPPED, PAUSED, RESTARTING, DESTROYED };

class ADFPipelineController;

/* Encapsulates the core functionalities of the ADF pipeline.
This includes constructing the pipeline and managing its lifecycle.
*/
class ADFPipeline {
 public:
  ADFPipeline(ADFPipelineController *parent) { parent_ = parent; }
  virtual ~ADFPipeline() {}

  void prepare();
  void start();
  void restart();
  void stop();
  void pause();
  void resume();
  void destroy();
  void stop_on_error();
  void force_destroy();

  PipelineState getState() { return state_; }
  void loop() { this->watch_(); }

  void set_destroy_on_stop(bool value){ this->destroy_on_stop_ = value; }
  void set_finish_timeout_ms(int timeout){ this->wait_for_finish_timeout_ms_ = timeout; }

  void append_element(ADFPipelineElement *element);
  int get_number_of_elements() { return pipeline_elements_.size(); }
  std::vector<std::string> get_element_names();
  void dump_element_configs();

  bool request_settings(AudioPipelineSettingsRequest &request);
  void on_settings_request_failed(AudioPipelineSettingsRequest request) {}

 protected:
  bool init_();
  bool deinit_();

  void set_state_(PipelineState state);

  void loop_();
  void watch_();
  bool check_all_created_();
  bool check_all_finished_();
  bool check_all_destroyed_();
  uint32_t finish_timeout_invoke_{0};
  int wait_for_finish_timeout_ms_{16000};

  enum CheckState { CHECK_PREPARED, CHECK_PAUSED, CHECK_RESUMED, CHECK_STOPPED, NUM_STATE_CHECKS };
  std::vector<std::string> check_state_name = {"PREPARING", "PAUSING", "RESUMING", "STOPPING","WRONG_IDX"};
  uint32_t check_timeout_invoke_[NUM_STATE_CHECKS] = {0};
  bool check_first_loop_[NUM_STATE_CHECKS] = {true};
  bool check_all_ready_[NUM_STATE_CHECKS] = {true};
  std::vector<ADFPipelineElement*>::iterator check_comp_it[NUM_STATE_CHECKS]{};

  template <ADFPipeline::CheckState E>
  bool call_and_check();

  void check_for_pipeline_events_();
  void forward_event_to_pipeline_elements_(audio_event_iface_msg_t &msg);

  bool build_adf_pipeline_();
  void deinit_all_();



  audio_pipeline_handle_t adf_pipeline_{};
  audio_event_iface_handle_t adf_pipeline_event_{};

  std::vector<ADFPipelineElement*> pipeline_elements_;

  ADFPipelineController *parent_{nullptr};

  PipelineState state_{PipelineState::UNINITIALIZED};
  PipelineRequest requested_{PipelineRequest::DESTROYED};

  bool destroy_on_stop_{false};
};


}  // namespace esp_adf
}  // namespace esphome

#endif
