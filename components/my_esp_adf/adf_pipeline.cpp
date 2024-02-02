#include "adf_pipeline.h"
#include "adf_audio_element.h"

#include "esphome/core/log.h"
namespace esphome {
namespace esp_adf {

static const char *const TAG = "esp_adf_pipeline";


void AudioPipeline::set_state_(PipelineState state){
    for( auto element& : pipeline_elements_ ){
        element->on_pipeline_status_change(state);
    }
    state_ = state;
}

bool AudioPipeline::request_settings(AudioPipelineSettingsRequest& request){
    for (auto it = pipeline_elements_.rbegin(); it != pipeline_elements_.rend(); ++it)
    {
        if(*it != request.requested_by){
            *it->request_settings(request);
        }
        
    }
    return !request.failed;
}


void AudioPipeline::init(){
    if( state_ == PipelineState::STATE_UNAVAILABLE && init_()){
        set_state_(PipelineState::STATE_STOPPED);
    }
}

void AudioPipeline::reset(){
    if( reset_() ){
        set_state(PipelineState::STATE_STOPPED);
    }
}

void AudioPipeline::start(){
    if( state_ == PipelineState::STATE_UNAVAILABLE ){
        init();
    }
    if( state_ == PipelineState::STATE_STOPPED && start_() ){
        set_state(PipelineState::STATE_RUNNING)
    }
         
}

void AudioPipeline::stop(){
    if( state_ == PipelineState::STATE_RUNNING && stop_() ){
        set_state(PipelineState::STATE_STOPPED)
    }
}

void AudioPipeline::pause(){
    if( state_ == PipelineState::STATE_RUNNING && pause_() ){
        set_state(PipelineState::STATE_PAUSED)
    }
}

void AudioPipeline::resume(){
    if( state_ == PipelineState::STATE_PAUSED && resume_() ){
        set_state(PipelineState::STATE_RUNNING)
    }
}

void AudioPipeline::append_element(AudioPipelineElement* element){
    const bool isFirst = pipeline_elements_.size() == 0;
    if( isFirst ){
        if( element->get_element_type != AUDIO_PIPELINE_SOURCE ){
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
}

std::vector<std::string> AudioPipeline::get_element_names(){
    std::vector<std::string> name_tags;
    for( auto element : pipeline_elements_ ){
        name_tags.push_back( element->get_name() );
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

void ADFPipeline::pause(){
    return audio_pipeline_pause(adf_pipeline_) == ESP_OK;
}

void ADFPipeline::resume(){
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
    for( auto &comp : pipeline_elements_ )
    {
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
        dynamic_cast<ADFPipelineElement*>(element)->adf_event_handler(msg);
    }
}

void ADFPipeline::watch_(){
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(this->adf_pipeline_event_, &msg, 0);
    if ( ret == ESP_OK ){
       forward_event_to_pipeline_elements_(msg); 
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
                    set_state(PipelineState::STATE_STOPPED);
                    this->reset();
                    break;
                case AEL_STATUS_STATE_RUNNING:
                    set_state(PipelineState::STATE_RUNNING);
                    break;
                case AEL_STATUS_STATE_PAUSED:
                    set_state(PipelineState::STATE_PAUSED);
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
       audio_element_handle_t mp3_decoder = this->audio_components_[0]->get_adf_elements()[1];
       if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void *) mp3_decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(mp3_decoder, &music_info);

            esph_log_i(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                     music_info.sample_rates, music_info.bits, music_info.channels);
            this->get_last_component()->set_sampling_frequency(music_info.sample_rates);
            this->get_last_component()->set_number_of_channels(music_info.channels);

        }
        

    }
}

}
}