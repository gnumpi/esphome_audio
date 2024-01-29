#include "adf_pipeline.h"
#include "esphome/core/log.h"
namespace esphome {
namespace esp_adf {

static const char *const TAG = "esp_adf_pipeline";

void ADFPipeline::append_component(ADFAudioComponent* comp){
    const bool isFirst = this->audio_components_.size() == 0;
    if( isFirst ){
        if( comp->hasInputBuffer() ){
            esph_log_e(TAG, "First component should not expect input data.");
            return;
        }
    }
    else {
        const bool can_append = this->audio_components_.back()->hasOutputBuffer();
        if( !can_append ){
          return;
        }
    }
    this->audio_components_.push_back(comp);
}


void ADFPipeline::start(){
    if( getState() == PipelineState::STATE_UNAVAILABLE ){
        init();
    }
    audio_pipeline_run(this->adf_pipeline_);
}

void ADFPipeline::stop(){
    terminate_pipeline_();
    reset();
    this->state_ = PipelineState::STATE_STOPPED;
}

void ADFPipeline::pause(){
    audio_pipeline_pause(this->adf_pipeline_);
}

void ADFPipeline::resume(){
    audio_pipeline_resume(this->adf_pipeline_);
}


void ADFPipeline::init(){
  if( !this->adf_pipeline_ ){
    this->build_adf_pipeline_();
    this->state_ = PipelineState::STATE_STOPPED;
  }
}

void ADFPipeline::reset(){
    audio_pipeline_handle_t pipeline = this->adf_pipeline_;
    audio_pipeline_reset_ringbuffer(pipeline);
    audio_pipeline_reset_elements(pipeline);
    audio_pipeline_change_state(pipeline, AEL_STATE_INIT );
}


void ADFPipeline::build_adf_pipeline_(){
    if( this->adf_pipeline_ != nullptr ){
        return;
    }
    audio_pipeline_cfg_t pipeline_cfg = {
      .rb_size = 8 * 1024,
    };
    this->adf_pipeline_ = audio_pipeline_init(&pipeline_cfg);
    
    std::vector<std::string> tags_vector;
    for( auto &comp : this->audio_components_ )
    {
       comp->init_adf_elements();
       int i = 0;
       for( auto el : comp->get_adf_elements() )
       {
          tags_vector.push_back( comp->get_adf_element_tag(i) );   
          audio_pipeline_register(this->adf_pipeline_, el , tags_vector.back().c_str() );
          i++;
       }
    }
    
    const char** link_tag_ptrs = new const char*[tags_vector.size()]; 
    for( int i = 0; i < tags_vector.size(); i++){
        link_tag_ptrs[i] = tags_vector[i].c_str();
        esph_log_d(TAG, "pipeline tag %d, %s", i, link_tag_ptrs[i] );
    }
    audio_pipeline_link(this->adf_pipeline_, link_tag_ptrs, tags_vector.size());
    
    delete link_tag_ptrs;
    this->adf_last_element_in_pipeline_ = this->audio_components_.back()->get_adf_elements().back();

    esph_log_d(TAG, "Setting up event listener." );
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    this->adf_pipeline_event_ = audio_event_iface_init(&evt_cfg);
    audio_pipeline_set_listener(this->adf_pipeline_, this->adf_pipeline_event_);
}

void ADFPipeline::terminate_pipeline_(){
  audio_pipeline_stop(this->adf_pipeline_);
  audio_pipeline_wait_for_stop(this->adf_pipeline_);
  audio_pipeline_terminate(this->adf_pipeline_);
}

void ADFPipeline::reset_states_(){
  audio_pipeline_reset_elements(this->adf_pipeline_);  
}

void ADFPipeline::deinit_all_(){
  audio_pipeline_handle_t pipeline = this->adf_pipeline_;
  this->terminate_pipeline_();
  audio_pipeline_remove_listener(pipeline);
  audio_pipeline_deinit(pipeline);
  this->adf_pipeline_ = nullptr;
  for( auto &comp : this->audio_components_ ){
    comp->deinit_adf_elements();
  }
  audio_event_iface_destroy(this->adf_pipeline_event_);
  this->adf_pipeline_event_ = nullptr;
}

void ADFPipeline::watch_(){
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(this->adf_pipeline_event_, &msg, 0);
    if ( ret == ESP_OK ){
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
                    this->state_ = PipelineState::STATE_STOPPED;
                    this->reset();
                    break;
                case AEL_STATUS_STATE_RUNNING:
                    this->state_ = PipelineState::STATE_RUNNING;
                    break;
                case AEL_STATUS_STATE_PAUSED:
                    this->state_ = PipelineState::STATE_PAUSED;
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