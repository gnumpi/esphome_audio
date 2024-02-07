#include "adf_pipeline.h"
#include "adf_audio_element.h"

#include "esphome/core/log.h"
namespace esphome {
namespace esp_adf {

static const char *const TAG = "esp_adf_pipeline";


void AudioPipeline::init(){
    if( state_ == PipelineState::UNAVAILABLE && init_()){
        set_state_(PipelineState::STOPPED);
    }
}

void AudioPipeline::reset(){
    if( reset_() ){
        set_state_(PipelineState::STOPPED);
    }
}

void AudioPipeline::start(){
    if( state_ == PipelineState::UNAVAILABLE ){
        init();
    }
    if( state_ == PipelineState::STOPPED && start_() ){
        set_state_(PipelineState::RUNNING);
    }
         
}

void AudioPipeline::stop(){
    if( state_ == PipelineState::RUNNING && stop_() ){
        set_state_(PipelineState::STOPPED);
    }
}

void AudioPipeline::pause(){
    if( state_ == PipelineState::RUNNING && pause_() ){
        set_state_(PipelineState::PAUSED);
    }
}

void AudioPipeline::resume(){
    if( state_ == PipelineState::PAUSED && resume_() ){
        set_state_(PipelineState::RUNNING);
    }
}


void ADFPipeline::append_element(ADFPipelineElement* element){
    const bool isFirst = pipeline_elements_.size() == 0;
    if( isFirst ){
        if( element->get_element_type() != AUDIO_PIPELINE_SOURCE ){
            esph_log_e(TAG, "First component should be a source element.");
            return;
        }
    }
    else {
        const AudioPipelineElementType last_type = pipeline_elements_.back()->get_element_type();
        if( last_type == AUDIO_PIPELINE_SINK ){
          esph_log_e(TAG, "Can't append to a sink component.");
          return;
        }
    }
    pipeline_elements_.push_back(element);
    element->set_pipeline(this);
}


std::vector<std::string> ADFPipeline::get_element_names(){
    std::vector<std::string> name_tags;
    for( auto element : pipeline_elements_ ){
        name_tags.push_back( element->get_name() );
    }
    return name_tags;
}

bool ADFPipeline::request_settings(AudioPipelineSettingsRequest& request){
    for ( auto it = pipeline_elements_.rbegin(); it != pipeline_elements_.rend(); ++it)
    {
        if(*it != request.requested_by){
            (*it)->on_settings_request(request);
        }
        
    }
    return !request.failed;
}

void ADFPipeline::set_state_(PipelineState state){
    state_ = state;
    for( auto element : pipeline_elements_ ){
        element->on_pipeline_status_change();
    }
    if( parent_ != nullptr ){
        parent_->on_pipeline_state_change(state);
    }
  }


bool ADFPipeline::init_(){
  if( !this->adf_pipeline_ ){
    return build_adf_pipeline_();
  }
  return false;
}

bool ADFPipeline::reset_(){
    audio_pipeline_handle_t pipeline = this->adf_pipeline_;
    return (
        audio_pipeline_reset_ringbuffer(pipeline) == ESP_OK
        && audio_pipeline_reset_elements(pipeline) == ESP_OK
        && audio_pipeline_change_state(pipeline, AEL_STATE_INIT ) == ESP_OK
    );
}

bool ADFPipeline::start_(){
    return audio_pipeline_run(adf_pipeline_) == ESP_OK;
}

bool ADFPipeline::stop_(){
    return terminate_pipeline_() && reset_();
}

bool ADFPipeline::pause_(){
    return audio_pipeline_pause(adf_pipeline_) == ESP_OK;
}

bool ADFPipeline::resume_(){
    return audio_pipeline_resume(adf_pipeline_) == ESP_OK;
}


bool ADFPipeline::build_adf_pipeline_(){
    if( adf_pipeline_ != nullptr ){
        return false;
    }
    audio_pipeline_cfg_t pipeline_cfg = {
      .rb_size = 8 * 1024,
    };
    adf_pipeline_ = audio_pipeline_init(&pipeline_cfg);
    
    std::vector<std::string> tags_vector;
    for( auto &comp_base : pipeline_elements_ )
    {
       ADFPipelineElement* comp = dynamic_cast<ADFPipelineElement*>(comp_base);
       comp->init_adf_elements();
       int i = 0;
       for( auto el : comp->get_adf_elements() )
       {
          tags_vector.push_back( comp->get_adf_element_tag(i) );   
          if(audio_pipeline_register(adf_pipeline_, el , tags_vector.back().c_str() ) != ESP_OK){
            deinit_all_();
            return false;
          }
          i++;
       }
    }
    
    const char** link_tag_ptrs = new const char*[tags_vector.size()]; 
    for( int i = 0; i < tags_vector.size(); i++){
        link_tag_ptrs[i] = tags_vector[i].c_str();
        esph_log_d(TAG, "pipeline tag %d, %s", i, link_tag_ptrs[i] );
    }
    
    if(audio_pipeline_link(adf_pipeline_, link_tag_ptrs, tags_vector.size()) != ESP_OK){
        delete link_tag_ptrs;
        deinit_all_();
        return false;    
    }
    delete link_tag_ptrs;
    
    adf_last_element_in_pipeline_ = pipeline_elements_.back()->get_adf_elements().back();

    esph_log_d(TAG, "Setting up event listener." );
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    adf_pipeline_event_ = audio_event_iface_init(&evt_cfg);
    if(audio_pipeline_set_listener(adf_pipeline_, adf_pipeline_event_) != ESP_OK){
        return false;
    }
    return true;
}

bool ADFPipeline::terminate_pipeline_(){
  return (
    audio_pipeline_stop(adf_pipeline_) == ESP_OK
    && audio_pipeline_wait_for_stop(adf_pipeline_) == ESP_OK
    && audio_pipeline_terminate(adf_pipeline_) == ESP_OK
  );
}


void ADFPipeline::deinit_all_(){
  terminate_pipeline_();
  audio_pipeline_remove_listener(adf_pipeline_);
  audio_pipeline_deinit(adf_pipeline_);
  adf_pipeline_ = nullptr;
  for( auto &comp : this->pipeline_elements_ ){
    comp->deinit_adf_elements();
  }
  audio_event_iface_destroy(adf_pipeline_event_);
  adf_pipeline_event_ = nullptr;
}

void ADFPipeline::forward_event_to_pipeline_elements_(audio_event_iface_msg_t &msg){
    for( auto &element : pipeline_elements_ )
    {
        element->sdk_event_handler_(msg);
    }
}

void ADFPipeline::watch_(){
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(this->adf_pipeline_event_, &msg, 0);
    if ( ret == ESP_OK ){
       forward_event_to_pipeline_elements_(msg); 
       if( parent_ != nullptr ){
         parent_->pipeline_event_handler(msg);
       }
       if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && msg.source == (void *) this->adf_last_element_in_pipeline_
        ){
            audio_element_status_t status;
            std::memcpy(&status, &msg.data, sizeof(audio_element_status_t));
            esph_log_i(TAG, "[ * ] CMD: %d  status: %d", msg.cmd, status );     
            switch(status){
                case AEL_STATUS_STATE_STOPPED:
                case AEL_STATUS_STATE_FINISHED:
                    set_state_(PipelineState::STOPPED);
                    this->reset();
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
       
       if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT){
            esph_log_i(TAG, "[ * ] CMD: %d Pipeline: %d", msg.cmd, this->state_ );     
       }
       if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT 
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
       ){
            audio_element_handle_t el = (audio_element_handle_t) msg.source;
            size_t status = reinterpret_cast<size_t> (msg.data);
            esph_log_i(TAG, "[ %s ] status: %d", audio_element_get_tag(el), status );     
       }
    }
}

}
}