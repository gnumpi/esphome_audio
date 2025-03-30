#include "adf_pipeline.h"

#include "adf_pipeline_controller.h"
#include "adf_audio_element.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace esp_adf {

static const char *const TAG = "esp_adf_pipeline";

static const uint32_t PIPELINE_PREPARATION_TIMEOUT_MS = 10000;

static const LogString *pipeline_state_to_string(PipelineState state) {
  switch (state) {
    case PipelineState::UNINITIALIZED:
      return LOG_STR("UNINITIALIZED");
    case PipelineState::INITIALIZING:
      return LOG_STR("INITIALIZING");
    case PipelineState::CREATED:
      return LOG_STR("CREATED");
    case PipelineState::PREPARING:
      return LOG_STR("PREPARING");
    case PipelineState::STOPPED:
      return LOG_STR("STOPPED");
    case PipelineState::STARTING:
      return LOG_STR("STARTING");
    case PipelineState::RUNNING:
      return LOG_STR("RUNNING");
    case PipelineState::FINISHING:
      return LOG_STR("FINISHING");
    case PipelineState::PAUSING:
      return LOG_STR("PAUSING");
    case PipelineState::PAUSED:
      return LOG_STR("PAUSED");
    case PipelineState::ABORTING:
      return LOG_STR("ABORTING");
    case PipelineState::DESTROYING:
      return LOG_STR("DESTROYING");
    default:
      return LOG_STR("UNKNOWN");
  }
}

void ADFPipeline::dump_element_configs(){
  for (auto &element : pipeline_elements_) {
    element->dump_config();
  }
}

void ADFPipeline::restart() {
  esph_log_d(TAG, "Starting request, current state %s", LOG_STR_ARG(pipeline_state_to_string(this->state_)));
  this->requested_ = PipelineRequest::RESTARTING;
  return;
}

void ADFPipeline::start() {
  esph_log_d(TAG, "Starting request, current state %s", LOG_STR_ARG(pipeline_state_to_string(this->state_)));
  if( this->state_  == PipelineState::PAUSED ){
    this->requested_ = PipelineRequest::RESTARTING;
    return;
  }

  this->requested_ = PipelineRequest::RUNNING;
}

void ADFPipeline::stop() {
  esph_log_d(TAG, "Called 'stop' while in %s state.",LOG_STR_ARG(pipeline_state_to_string(this->state_)) );
  this->requested_ = PipelineRequest::STOPPED;
}

void ADFPipeline::pause() {
  esph_log_d(TAG, "Calling pause! Current state: %s",LOG_STR_ARG(pipeline_state_to_string(this->state_)) );
  this->requested_ = PipelineRequest::PAUSED;

}

void ADFPipeline::resume() {
  esph_log_d(TAG, "Calling resume! Current state: %s",LOG_STR_ARG(pipeline_state_to_string(this->state_)) );
  this->requested_ = PipelineRequest::RUNNING;
}

void ADFPipeline::destroy() {
  esph_log_d(TAG, "Calling destroy! Current state: %s",LOG_STR_ARG(pipeline_state_to_string(this->state_)) );
  this->destroy_on_stop_ = true;
  this->requested_ = PipelineRequest::STOPPED;
}


bool ADFPipeline::check_all_finished_(){
  this->finish_timeout_invoke_ = millis();
  bool stopped = true;
  for (auto &comp : pipeline_elements_) {
    stopped &= comp->elements_have_stopped();
    if( !stopped ){
      return false;
    }
  }
  return true;
}



void ADFPipeline::check_for_pipeline_events_(){
  audio_event_iface_msg_t msg;
  esp_err_t ret = audio_event_iface_listen(this->adf_pipeline_event_, &msg, 0);

  if (ret == ESP_OK) {
    forward_event_to_pipeline_elements_(msg);

    if (parent_ != nullptr) {
      parent_->pipeline_event_handler(msg);
    }

    assert( this->state_ != PipelineState::PREPARING );

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.cmd == AEL_MSG_CMD_REPORT_POSITION){
     audio_element_info_t music_info{};
     audio_element_handle_t el = (audio_element_handle_t) msg.source;
     audio_element_getinfo(el, &music_info);
     esph_log_i(TAG, "[ %s ] byte_pos: %lld, total: %lld", audio_element_get_tag(el), music_info.byte_pos, music_info.total_bytes);
    }


    //trigger state changes on events received from pipeline
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.cmd == AEL_MSG_CMD_REPORT_STATUS){
      audio_element_status_t status;
      std::memcpy(&status, &msg.data, sizeof(audio_element_status_t));
      audio_element_handle_t el = (audio_element_handle_t) msg.source;
      esph_log_i(TAG, "[ %s ] status: %d", audio_element_get_tag(el), status);
      switch (status) {
        case AEL_STATUS_STATE_FINISHED:
          esph_log_i(TAG, "current state: %s",LOG_STR_ARG(pipeline_state_to_string(this->state_)) );
          if(    this->state_ == PipelineState::RUNNING
              || this->state_ == PipelineState::STARTING )
          {
            this->set_state_(PipelineState::FINISHING);
            if( check_all_finished_() )
            {
              this->requested_ = PipelineRequest::STOPPED;
            }
          }
          else if( this->state_ == PipelineState::FINISHING )
          {
            if( check_all_finished_() )
            {
              this->requested_ = PipelineRequest::STOPPED;
            }
          }
          break;

        case AEL_STATUS_STATE_STOPPED:
        case AEL_STATUS_STATE_RUNNING:
          break;

      case AEL_STATUS_ERROR_OPEN:
      case AEL_STATUS_ERROR_INPUT:
      case AEL_STATUS_ERROR_PROCESS:
      case AEL_STATUS_ERROR_OUTPUT:
      case AEL_STATUS_ERROR_CLOSE:
      case AEL_STATUS_ERROR_TIMEOUT:
      case AEL_STATUS_ERROR_UNKNOWN:
        this->stop_on_error();
        break;

      default:
        break;
      }
    }
  }
}

bool ADFPipeline::check_all_created_(){
  if( adf_pipeline_ == nullptr ){
    if( init_() ){
      return true;
    }
  }
  return false;
}


bool ADFPipeline::check_all_destroyed_(){
  return deinit_();
}


template <ADFPipeline::CheckState E>
bool ADFPipeline::call_and_check() {
  uint32_t &timeout_invoke = check_timeout_invoke_[E];
  bool &first_loop = check_first_loop_[E];
  bool &all_ready = check_all_ready_[E];
  std::vector<ADFPipelineElement*>::iterator &check_it = check_comp_it[E];

  if(timeout_invoke == 0){
    timeout_invoke = millis();
    check_it = this->pipeline_elements_.begin();
    first_loop = true;
  }

  switch(E){
    case CHECK_PREPARED:
      all_ready &= (*check_it)->prepare_elements(first_loop);
      //esph_log_d(TAG, "%s reported. All ready? %s", (*check_it)->get_name().c_str(),all_ready ? "yes" : "no" );
      break;
    case CHECK_PAUSED:
      all_ready &= (*check_it)->pause_elements(first_loop);
      break;
    case CHECK_RESUMED:
      all_ready &= (*check_it)->resume_elements(first_loop);
      break;
    case CHECK_STOPPED:
      all_ready &= (*check_it)->stop_elements(first_loop);
      break;
    default:
      break;
  };

  if( (*check_it)->in_error_state() )
  {
    esph_log_e(TAG, "%s got in error state while %s. Stopping pipeline!", (*check_it)->get_name().c_str(),check_state_name[E].c_str() );
    timeout_invoke = 0;
    if( !(E == CHECK_STOPPED) ){
      this->stop_on_error();
    } else {
      this->force_destroy();
    }
  }

  check_it++;
  if(check_it == this->pipeline_elements_.end())
  {
    check_it = this->pipeline_elements_.begin();
    if( all_ready ){
      timeout_invoke = 0;
      return true;
    }
    all_ready = true;
    first_loop = false;
  }

  if( millis() - timeout_invoke > 6000 )
  {
    esph_log_e(TAG, "Timeout while %s. Stopping pipeline!", check_state_name[E].c_str() );
    timeout_invoke = 0;
    if( !(E == CHECK_STOPPED) ){
      this->stop_on_error();
    } else {
      this->force_destroy();
    }
  }

  return false;
}

template bool ADFPipeline::call_and_check<ADFPipeline::CHECK_PREPARED>();
template bool ADFPipeline::call_and_check<ADFPipeline::CHECK_PAUSED>();
template bool ADFPipeline::call_and_check<ADFPipeline::CHECK_RESUMED>();
template bool ADFPipeline::call_and_check<ADFPipeline::CHECK_STOPPED>();


void ADFPipeline::stop_on_error(){
  this->requested_ = PipelineRequest::STOPPED;
  this->set_state_(PipelineState::ABORTING);
}

void ADFPipeline::force_destroy(){
  this->requested_ = PipelineRequest::DESTROYED;
  this->set_state_(PipelineState::DESTROYING);
}


void ADFPipeline::watch_() {
  switch(this->state_){

    case PipelineState::UNINITIALIZED:
      if(    this->requested_ == PipelineRequest::RUNNING
          || this->requested_ == PipelineRequest::RESTARTING ){
        set_state_(PipelineState::INITIALIZING);
        return;
      }
      break;

    case PipelineState::CREATED:
      // intended for pipeline creation during setup
      if(    this->requested_ == PipelineRequest::RUNNING
          || this->requested_ == PipelineRequest::RESTARTING ){
        set_state_(PipelineState::PREPARING);
      }
      break;

    case PipelineState::STOPPED:
      if(    this->requested_ == PipelineRequest::RUNNING
          || this->requested_ == PipelineRequest::RESTARTING ){
        set_state_(PipelineState::PREPARING);
        return;
      }
      if( this->destroy_on_stop_){
        set_state_(PipelineState::DESTROYING);
        return;
      }
      break;

    case PipelineState::PAUSED:
      if( this->requested_ == PipelineRequest::RESTARTING){
        set_state_(PipelineState::ABORTING);
        this->requested_ = PipelineRequest::RUNNING;
        return;
      }
      if( this->requested_ == PipelineRequest::RUNNING){
        set_state_(PipelineState::STARTING);
        return;
      }
      if( this->requested_ == PipelineRequest::STOPPED){
        set_state_(PipelineState::ABORTING);
        return;
      }
      break;

    case PipelineState::RUNNING:
      check_for_pipeline_events_();
      if( this->requested_ == PipelineRequest::STOPPED ){
        set_state_(PipelineState::ABORTING);
      }
      else if (this->requested_ == PipelineRequest::PAUSED){
        set_state_(PipelineState::PAUSING);
      }
      break;

    case PipelineState::INITIALIZING:
      if( check_all_created_()){
        set_state_(PipelineState::CREATED);
        return;
      }
      set_state_(PipelineState::DESTROYING);
      break;

    case PipelineState::PREPARING:
      if ( call_and_check<CHECK_PREPARED>() ){
        esph_log_d(TAG, "wait for preparation, done");
        if( this->requested_ == PipelineRequest::RESTARTING ){
          this->requested_ = PipelineRequest::RUNNING;
        }
        if( this->requested_ == PipelineRequest::RUNNING ){
            set_state_(PipelineState::STARTING);
            return;
        }
        this->set_state_(PipelineState::ABORTING);
      }
      {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(this->adf_pipeline_event_, &msg, 0);
        if (ret == ESP_OK) {
          forward_event_to_pipeline_elements_(msg);
        }
      }
      break;

    case PipelineState::STARTING:
      if( call_and_check<CHECK_RESUMED>() ){
        set_state_(PipelineState::RUNNING);
      }
      check_for_pipeline_events_();
      break;


    case PipelineState::FINISHING:
      check_for_pipeline_events_();
      if(this->requested_ == PipelineRequest::STOPPED){
        set_state_(PipelineState::STOPPED);
      }
      else if ( millis() - this->finish_timeout_invoke_ > this->wait_for_finish_timeout_ms_){
        this->requested_ = PipelineRequest::STOPPED;
        set_state_(PipelineState::ABORTING);
      }
      break;

    case PipelineState::PAUSING:
      if( call_and_check<CHECK_PAUSED>() ){
        set_state_(PipelineState::PAUSED);
      }
      break;

    case PipelineState::ABORTING:
      if( call_and_check<CHECK_STOPPED>() ) {
        set_state_(PipelineState::STOPPED);
      }
      break;

    case PipelineState::DESTROYING:
      if( check_all_destroyed_() ){
        set_state_(PipelineState::UNINITIALIZED);
      }
      break;
  }
}

void ADFPipeline::forward_event_to_pipeline_elements_(audio_event_iface_msg_t &msg) {
  for (auto &element : pipeline_elements_) {
    element->sdk_event_handler_(msg);
  }
}

void ADFPipeline::append_element(ADFPipelineElement *element) {
  const bool isFirst = pipeline_elements_.size() == 0;
  if (isFirst) {
    if (element->get_element_type() != AUDIO_PIPELINE_SOURCE) {
      esph_log_e(TAG, "First component should be a source element.");
      return;
    }
  } else {
    const AudioPipelineElementType last_type = pipeline_elements_.back()->get_element_type();
    if (last_type == AUDIO_PIPELINE_SINK) {
      esph_log_e(TAG, "Can't append to a sink component.");
      return;
    }
  }
  pipeline_elements_.push_back(element);
  element->set_pipeline(this);
}

std::vector<std::string> ADFPipeline::get_element_names() {
  std::vector<std::string> name_tags;
  for (auto element : pipeline_elements_) {
    name_tags.push_back(element->get_name());
  }
  return name_tags;
}

bool ADFPipeline::request_settings(AudioPipelineSettingsRequest &request) {
  for (auto it = pipeline_elements_.rbegin(); it != pipeline_elements_.rend(); ++it) {
    if (*it != request.requested_by) {
      (*it)->on_settings_request(request);
    }
    else {
      break;
    }
  }
  return !request.failed;
}

void ADFPipeline::set_state_(PipelineState state) {
  esph_log_d(TAG, "[%s] Pipeline changed from %s to %s. (REQ: %d)",
    this->parent_->get_name().c_str(),
    LOG_STR_ARG(pipeline_state_to_string(this->state_)),
             LOG_STR_ARG(pipeline_state_to_string(state)), (int) this->requested_);
  state_ = state;
  for (auto element : pipeline_elements_) {
    element->on_pipeline_status_change();
  }
  if (parent_ != nullptr) {
    parent_->on_pipeline_state_change(state);
  }
}

bool ADFPipeline::init_() { return build_adf_pipeline_(); }


bool ADFPipeline::deinit_() {
  this->deinit_all_();
  return true;
}

bool ADFPipeline::build_adf_pipeline_() {
  if (adf_pipeline_ != nullptr) {
    esph_log_d(TAG, "Pipline was already built.");
    return true;
  }
  audio_pipeline_cfg_t pipeline_cfg = {
      .rb_size = 8 * 1024,
  };
  adf_pipeline_ = audio_pipeline_init(&pipeline_cfg);

  std::vector<std::string> tags_vector;
  for (auto &comp : pipeline_elements_) {
    if (! comp->init_adf_elements() ){
      esph_log_e(TAG, "Couldn't init [%s].", comp->get_name().c_str() ) ;
      return false;
    }
    this->destroy_on_stop_ = this->destroy_on_stop_ || comp->requires_destruction_on_stop();
    int i = 0;
    for (auto el : comp->get_adf_elements()) {
      tags_vector.push_back(comp->get_adf_element_tag(i));
      if (audio_pipeline_register(adf_pipeline_, el, tags_vector.back().c_str()) != ESP_OK) {
        esph_log_e(TAG, "Couldn't register [%s] with pipeline", tags_vector.back().c_str());
        return false;
      }
      i++;
    }
  }

  const char **link_tag_ptrs = new const char *[tags_vector.size()];
  for (int i = 0; i < tags_vector.size(); i++) {
    link_tag_ptrs[i] = tags_vector[i].c_str();
    esph_log_d(TAG, "pipeline tag %d, %s", i, link_tag_ptrs[i]);
  }

  if (audio_pipeline_link(adf_pipeline_, link_tag_ptrs, tags_vector.size()) != ESP_OK) {
    delete[] link_tag_ptrs;
    esph_log_e(TAG, "Couldn't link pipeline elements");
    return false;
  }
  delete[] link_tag_ptrs;

  esph_log_d(TAG, "Setting up event listener.");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  adf_pipeline_event_ = audio_event_iface_init(&evt_cfg);
  if (audio_pipeline_set_listener(adf_pipeline_, adf_pipeline_event_) != ESP_OK) {
    esph_log_e(TAG, "Couldn't setup pipeline event listener");
    return false;
  }
  return true;
}

void ADFPipeline::deinit_all_() {
  esph_log_d(TAG, "Called deinit_all" );
  audio_pipeline_deinit(this->adf_pipeline_);
  // includes:
  //  audio_pipeline_terminate(pipeline);
  //  audio_pipeline_unlink(pipeline);
  //  for each el:
  //    audio_element_deinit
  //    audio_pipeline_unregister
  //  mutex_destroy
  //  audio_free

  if ( this->adf_pipeline_event_){
    audio_event_iface_destroy(this->adf_pipeline_event_);
  }

  for (auto &comp : this->pipeline_elements_) {
    comp->destroy_adf_elements();
  }
  this->adf_pipeline_ = nullptr;
  this->adf_pipeline_event_ = nullptr;
}

}  // namespace esp_adf
}  // namespace esphome
