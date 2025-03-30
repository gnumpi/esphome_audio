#include "adf_audio_element.h"
#ifdef USE_ESP_IDF

#include "sdk_ext.h"
namespace esphome {
namespace esp_adf {

const static int STOPPED_BIT = BIT0;
const static int STARTED_BIT = BIT1;
const static int BUFFER_REACH_LEVEL_BIT = BIT2;
const static int TASK_CREATED_BIT = BIT3;
const static int TASK_DESTROYED_BIT = BIT4;
const static int PAUSED_BIT = BIT5;
const static int RESUMED_BIT = BIT6;

static const char *const TAG = "adf_audio_element";

// make sure that element task is not running when setting a new state
static esp_err_t audio_element_force_set_state(audio_element_handle_t el, audio_element_state_t new_state)
{
    el->state = new_state;
    return ESP_OK;
}

static esp_err_t audio_element_cmd_send(audio_element_handle_t el, audio_element_msg_cmd_t cmd)
{
    audio_event_iface_msg_t msg = {
        .cmd = cmd,
        .data = nullptr,
        .data_len = 0,
        .source = el,
        .source_type = AUDIO_ELEMENT_TYPE_ELEMENT,
        .need_free_data = false
    };
    ESP_LOGV(TAG, "[%s]evt internal cmd = %d", el->tag, msg.cmd);
    return audio_event_iface_cmd(el->iface_event, &msg);
}


std::string ADFPipelineElement::get_adf_element_tag(int element_indx) {
  if (element_indx >= 0 && element_indx < this->sdk_element_tags_.size()) {
    return this->sdk_element_tags_[element_indx];
  }
  return "Unknown";
}

void ADFPipelineElement::clear_adf_elements_() {
  this->sdk_audio_elements_.clear();
  this->sdk_element_tags_.clear();
}


bool ADFPipelineElement::pause_elements(bool initial_call){
  if( initial_call ){
    all_paused_ = false;
    for( auto el : this->sdk_audio_elements_ ){
      esph_log_d(TAG, "Pausing [%s]...", el->tag );
      if ( el->state >= AEL_STATE_PAUSED){
        xEventGroupSetBits(el->state_event, PAUSED_BIT);
      }
      else if ( el->task_stack <= 0) {
        el->is_running = false;
        audio_element_force_set_state(el, AEL_STATE_PAUSED);
        xEventGroupSetBits(el->state_event, PAUSED_BIT);
      }
      else {
        audio_element_abort_output_ringbuf(el);
        audio_element_abort_input_ringbuf(el);
        xEventGroupClearBits(el->state_event, PAUSED_BIT);
        audio_event_iface_set_cmd_waiting_timeout(el->iface_event, portMAX_DELAY);
        if (audio_element_cmd_send(el, AEL_MSG_CMD_PAUSE) != ESP_OK) {
          ESP_LOGE(TAG, "[%s] Element send cmd error when AUDIO_ELEMENT_PAUSE", el->tag);
          this->element_state_ = PipelineElementState::ERROR;
          return false;
        }
      }
    }
    return false;
  }

  if ( all_paused_ ){
    return true;
  }

  for( auto el : this->sdk_audio_elements_ ){
    EventBits_t uxBits = xEventGroupGetBits( el->state_event );
    if ((uxBits & PAUSED_BIT) != PAUSED_BIT) {
      esph_log_d(TAG, "[%s] Checking Pause State, got %ld", el->tag, uxBits);
      return false;
    }
  }

  for( auto el : this->sdk_audio_elements_ ){
    audio_element_reset_input_ringbuf(el);
    audio_element_reset_output_ringbuf(el);
  }

  all_paused_ = true;

  return true;
}


bool ADFPipelineElement::prepare_elements(bool initial_call){
  if( initial_call ){
    all_prepared_ = false;
    for( auto el : this->sdk_audio_elements_ ){
      esph_log_d(TAG, "Preparing [%s]...", el->tag );
      if (!el->task_run) {
        if( audio_element_run(el) != ESP_OK )
        {
          esph_log_e(TAG, "Starting [%s] task failed.", el->tag );
          this->element_state_ = PipelineElementState::ERROR;
        }
      }
      else if( audio_element_get_state(el) == AEL_STATE_PAUSED || audio_element_get_state(el) == AEL_STATE_RUNNING){
        audio_element_stop(el);
      }
      if( audio_element_reset_state(el) != ESP_OK )
      {
        esph_log_e(TAG, "Resetting [%s] failed.", el->tag );
        this->element_state_ = PipelineElementState::ERROR;
      }
    }
    return false;
  }

  if ( all_prepared_ ){
    return true;
  }

  if( !this->is_ready() ){
    return false;
  }

  for( auto el : this->sdk_audio_elements_ ){
    if( audio_element_get_state(el) == AEL_STATE_STOPPED){
      if( audio_element_reset_state(el) != ESP_OK )
      {
        esph_log_e(TAG, "Resetting [%s] failed.", el->tag );
        this->element_state_ = PipelineElementState::ERROR;
      }
      return false;
    }
    if( audio_element_get_state(el) != AEL_STATE_INIT){
      esph_log_d(TAG, "[%s] Checking State, got %d", el->tag,audio_element_get_state(el) );
      return false;
    }
  }

  all_prepared_ = true;

  for( auto el : this->sdk_audio_elements_ ){
    audio_element_reset_input_ringbuf(el);
    audio_element_reset_output_ringbuf(el);
  }
  return true;
}


bool ADFPipelineElement::resume_elements(bool initial_call){
  if( initial_call ){
    all_resumed_ = false;
    for( auto el : this->sdk_audio_elements_ ){
      esph_log_d(TAG, "Resuming [%s]...", el->tag);
      if(el->task_stack <= 0 ){
        el->is_running = true;
        audio_element_force_set_state(el, AEL_STATE_RUNNING);
        xEventGroupSetBits(el->state_event, RESUMED_BIT);
      }
      else {
        esph_log_d(TAG, "[%s] Sending resume command.", el->tag);
        int ret =  ESP_OK;
        xEventGroupClearBits(el->state_event, RESUMED_BIT);
        if (audio_element_cmd_send(el, AEL_MSG_CMD_RESUME) == ESP_FAIL) {
          ESP_LOGW(TAG, "[%s] Send resume command failed", el->tag);
          this->element_state_ = PipelineElementState::ERROR;
          return false;
        }
      }
    }
    return false;
  }

  if ( all_resumed_ ){
    return true;
  }

  for( auto el : this->sdk_audio_elements_ ){
    EventBits_t uxBits = xEventGroupGetBits( el->state_event );
    esph_log_d(TAG, "[%s] Checking State, got %ld", el->tag, uxBits);
    if ((uxBits & RESUMED_BIT) != RESUMED_BIT) {
      return false;
    }
  }

  all_resumed_ = true;
  return true;
}

/*
proecess_running:
  - if !is_running
    - return
  - call el->process


audio_element_run
  - if task_element:
      - task gets created

  - if non_task_element:
      - AEL_STATE_RUNNING is set
      - task_run = true
      - is_running = true


audio_element_resume
  - if task_element:
      - wait_for_finished: AEL_MSG_CMD_RESUME
          - if state == RUNNING
              - is_running = true
              - return
          - if state in [INITIALIZING, STOPPED, FINISHED, ERROR]:
              - reset_output_ringbuf

          - is_running = true
          - call preocess_init
              - if el has open cb
                  - set state to INITIALIZING
                  - call el->open
                  - >> set state to either (RUNNING or ERROR) <<

              - if el doesn't have open cb: >> state is NOT set <<

              - if FAIL :
                  - abort_output_ringbuf
                  - abort_input_ringbuf
                  - is_running = false

          - cmd_waiting_time = 0
          - clear STOPPED_BIT

  - if non_task_element:
      - AEL_STATE_RUNNING is set
      - is_running = true


audio_element_stop:
  - abort_output_ringbuf
  - abort_input_ringbuf

  if task_element:
    - set stopping = true
    - send AEL_MSG_CMD_STOP
        - if state in [FINISHED, STOPPED]:
            - skip reporting and STOPPED_BIT if already stopped
            - set state to STOPPED
            - set is_running to false
            - set stopping to false
            - set STOPPED_BIT
        - else:
            - call process_deinit
                - call el->close
            - set is_running to false
            - set stopping to false
            - set STOPPED_BIT

  if not task_element:
    - is_running = false
    - set state to STOPPED
    - set STOPPED_BIT


audio_element_pause: (difference to stop: waits for el->process to finish)
  - if state in [PAUSED, STOPPED, FINISHED, ERROR]
      - set state to PAUSED
      - return

  - if not task_element:
      - set is_running = false
      - set state to PAUSED
      - return

  - send AEL_MSG_CMD_PAUSE
  - wait for AEL_MSG_CMD_PAUSE finished
        - set state to PAUSED
        - call process_deinit
            - call el->close
        - set cmd waiting timeout to portMAX_DELAY
        - report AEL_STATE_PAUSED
        - set is_running false
        - set PAUSED_BIT


task thread gets destroyed with AEL_MSG_CMD_DESTROY (audio_element_terminate)


AEL_STATE_PAUSED        = 4,
  - process_deinit
  - is_running = false


AEL_STATE_NONE          = 0,
AEL_STATE_INIT          = 1,
AEL_STATE_INITIALIZING  = 2,
AEL_STATE_RUNNING       = 3,
AEL_STATE_PAUSED        = 4,
AEL_STATE_STOPPED       = 5,
AEL_STATE_FINISHED      = 6,
AEL_STATE_ERROR         = 7
*/



bool ADFPipelineElement::stop_elements(bool initial_call){
  if( initial_call){
    all_stopped_ = true;
    for( auto el : this->sdk_audio_elements_ ){
      if( audio_element_get_state(el) == AEL_STATE_STOPPED || audio_element_get_state(el) == AEL_STATE_FINISHED || audio_element_get_state(el) == AEL_STATE_INIT ){
        continue;
      }
      else {
        esph_log_d(TAG, "[%s] Checking State for stopping, got %d", el->tag, audio_element_get_state(el) );
      }
      all_stopped_ = false;
      if( audio_element_stop(el) != ESP_OK){
        ESP_LOGW(TAG, "[%s] Send resume command failed", el->tag);
        this->element_state_ = PipelineElementState::ERROR;
        return false;
      }
    }
    return all_stopped_;
  }

  if(all_stopped_){
    return true;
  }

  for( auto el : this->sdk_audio_elements_ ){
    EventBits_t uxBits = xEventGroupWaitBits(el->state_event, STOPPED_BIT, false, true, 0);
    if ((uxBits & STOPPED_BIT) == 0) {
        return false;
    }
  }
  all_stopped_ = true;
  return true;
}

bool ADFPipelineElement::elements_have_stopped(){
   for( auto el : this->sdk_audio_elements_ ){
    if( audio_element_get_state(el) < AEL_STATE_PAUSED ){
        return false;
      }
  }
  return true;
}


}  // namespace esp_adf
}  // namespace esphome

#endif
