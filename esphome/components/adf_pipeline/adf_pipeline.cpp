#include "adf_pipeline.h"
#include "adf_audio_element.h"

#include "esphome/core/log.h"
namespace esphome {
namespace esp_adf {

static const char *const TAG = "esp_adf_pipeline";

static const LogString *pipeline_state_to_string(PipelineState state) {
  switch (state) {
    case PipelineState::UNAVAILABLE:
      return LOG_STR("UNAVAILABLE");
    case PipelineState::PREPARING:
      return LOG_STR("PREPARING");
    case PipelineState::STARTING:
      return LOG_STR("STARTING");
    case PipelineState::RUNNING:
      return LOG_STR("RUNNING");
    case PipelineState::STOPPING:
      return LOG_STR("STOPPING");
    case PipelineState::STOPPED:
      return LOG_STR("STOPPED");
    case PipelineState::PAUSING:
      return LOG_STR("PAUSING");
    case PipelineState::PAUSED:
      return LOG_STR("PAUSED");
    case PipelineState::RESUMING:
      return LOG_STR("RESUMING");
    case PipelineState::DESTROYING:
      return LOG_STR("DESTROYING");
    default:
      return LOG_STR("UNKNOWN");
  }
}

void ADFPipeline::init() {
  esph_log_d(TAG, "Init request, current state %s", LOG_STR_ARG(pipeline_state_to_string(this->state_)));
  if (state_ == PipelineState::UNAVAILABLE) {
    if (build_adf_pipeline_()) {
      set_state_(PipelineState::STOPPED);
    } else {
      esph_log_e(TAG, "Couldn't init pipeline");
    }
  }
}

void ADFPipeline::start() {
  esph_log_d(TAG, "Starting request, current state %s", LOG_STR_ARG(pipeline_state_to_string(this->state_)));
  if (state_ == PipelineState::UNAVAILABLE) {
    init();
  }
  if (state_ == PipelineState::STOPPED || state_ == PipelineState::PAUSED || state_ == PipelineState::STOPPING) {
    set_state_(PipelineState::PREPARING);
  }
}

void ADFPipeline::stop() {
  if (state_ == PipelineState::RUNNING || state_ == PipelineState::PAUSED || state_ == PipelineState::STARTING) {
    set_state_(PipelineState::STOPPING);
    stop_();
  }
}

void ADFPipeline::pause() {
  if (state_ == PipelineState::RUNNING) {
    set_state_(PipelineState::PAUSING);
    pause_();
  }
}

void ADFPipeline::resume() {
  if (state_ == PipelineState::PAUSED) {
    set_state_(PipelineState::RESUMING);
    resume_();
  }
}

void ADFPipeline::destroy() {
  if (state_ == PipelineState::STOPPED) {
    this->deinit_all_();
  }
}

void ADFPipeline::check_if_components_are_ready_(){
    bool ready = true;
    for (auto &element : this->pipeline_elements_) {
      ready = ready && element->isReady();
    }
    if (ready) {
      this->set_state_(PipelineState::STARTING);
      this->start_();
    }
  audio_event_iface_msg_t msg;
  esp_err_t ret = audio_event_iface_listen(this->adf_pipeline_event_, &msg, 0);
  if (ret == ESP_OK) {
    forward_event_to_pipeline_elements_(msg);
  }
  }

void ADFPipeline::check_for_pipeline_events_(){
  audio_event_iface_msg_t msg;
  esp_err_t ret = audio_event_iface_listen(this->adf_pipeline_event_, &msg, 0);
  if (ret == ESP_OK) {
    forward_event_to_pipeline_elements_(msg);

      if (parent_ != nullptr) {
        parent_->pipeline_event_handler(msg);
      }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {
      audio_element_handle_t el = (audio_element_handle_t) msg.source;
      size_t status = reinterpret_cast<size_t>(msg.data);
      esph_log_i(TAG, "[ %s ] status: %d", audio_element_get_tag(el), status);
    }

    //trigger state changes on events received from the last element in pipeline
      if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.cmd == AEL_MSG_CMD_REPORT_STATUS &&
          msg.source == (void *) this->adf_last_element_in_pipeline_) {
        audio_element_status_t status;
        std::memcpy(&status, &msg.data, sizeof(audio_element_status_t));
        esph_log_i(TAG, "[ * ] CMD: %d  status: %d", msg.cmd, status);
        switch (status) {
          case AEL_STATUS_STATE_STOPPED:
          case AEL_STATUS_STATE_FINISHED:
            this->reset_();
            set_state_(PipelineState::STOPPED);
            break;
          case AEL_STATUS_STATE_RUNNING:
            set_state_(PipelineState::RUNNING);
            break;
          case AEL_STATUS_STATE_PAUSED:
            set_state_(PipelineState::PAUSED);
            break;
          default:
            break;
        }
      }
    }
    }

void ADFPipeline::watch_() {
  switch(this->state_){
    case PipelineState::UNAVAILABLE:
    case PipelineState::STOPPED:
      break;
    case PipelineState::PREPARING:
      check_if_components_are_ready_();
      break;
    default:
      check_for_pipeline_events_();
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
  }
  return !request.failed;
}

void ADFPipeline::set_state_(PipelineState state) {
  esph_log_d(TAG, "State changed from %s to %s", LOG_STR_ARG(pipeline_state_to_string(this->state_)),
             LOG_STR_ARG(pipeline_state_to_string(state)));
  state_ = state;
  for (auto element : pipeline_elements_) {
    element->on_pipeline_status_change();
  }
  if (parent_ != nullptr) {
    parent_->on_pipeline_state_change(state);
  }
}

bool ADFPipeline::init_() { return build_adf_pipeline_(); }

bool ADFPipeline::start_() { return audio_pipeline_run(adf_pipeline_) == ESP_OK; }

bool ADFPipeline::stop_() { return audio_pipeline_stop(adf_pipeline_) == ESP_OK; }

bool ADFPipeline::pause_() { return audio_pipeline_pause(adf_pipeline_) == ESP_OK; }

bool ADFPipeline::resume_() { return audio_pipeline_resume(adf_pipeline_) == ESP_OK; }

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
  for (auto &comp_base : pipeline_elements_) {
    ADFPipelineElement *comp = dynamic_cast<ADFPipelineElement *>(comp_base);
    comp->init_adf_elements();
    int i = 0;
    for (auto el : comp->get_adf_elements()) {
      tags_vector.push_back(comp->get_adf_element_tag(i));
      if (audio_pipeline_register(adf_pipeline_, el, tags_vector.back().c_str()) != ESP_OK) {
        esph_log_e(TAG, "Couldn't register [%s] with pipeline", tags_vector.back().c_str());
        deinit_all_();
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
    delete link_tag_ptrs;
    deinit_all_();
    esph_log_e(TAG, "Couldn't link pipeline elements");
    return false;
  }
  delete link_tag_ptrs;

  adf_last_element_in_pipeline_ = pipeline_elements_.back()->get_adf_elements().back();

  esph_log_d(TAG, "Setting up event listener.");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  adf_pipeline_event_ = audio_event_iface_init(&evt_cfg);
  if (audio_pipeline_set_listener(adf_pipeline_, adf_pipeline_event_) != ESP_OK) {
    esph_log_e(TAG, "Couldn't setup pipeline event listener");
    return false;
  }
  return true;
}

bool ADFPipeline::reset_ringbuffer() {
  audio_pipeline_handle_t pipeline = this->adf_pipeline_;
  return (audio_pipeline_reset_ringbuffer(pipeline) == ESP_OK);
}

bool ADFPipeline::reset_() {
  audio_pipeline_handle_t pipeline = this->adf_pipeline_;
  bool ret =  (audio_pipeline_reset_ringbuffer(pipeline) == ESP_OK && audio_pipeline_reset_elements(pipeline) == ESP_OK &&
          audio_pipeline_change_state(pipeline, AEL_STATE_INIT) == ESP_OK);
  this->set_state_(PipelineState::STOPPED);
  return ret;
}

void ADFPipeline::deinit_all_() {
  esph_log_d(TAG, "Called deinit_all" );
  audio_pipeline_wait_for_stop(this->adf_pipeline_);
  audio_pipeline_deinit(this->adf_pipeline_);
  audio_event_iface_destroy(this->adf_pipeline_event_);
  for (auto &comp : this->pipeline_elements_) {
    comp->deinit_adf_elements();
  }
  this->adf_pipeline_ = nullptr;
  this->adf_pipeline_event_ = nullptr;
  this->set_state_(PipelineState::UNAVAILABLE);
}

void ADFPipeline::forward_event_to_pipeline_elements_(audio_event_iface_msg_t &msg) {
  for (auto &element : pipeline_elements_) {
    element->sdk_event_handler_(msg);
  }
}

}  // namespace esp_adf
}  // namespace esphome
