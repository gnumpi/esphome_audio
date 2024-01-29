#pragma once

#include <cstring>
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF 

#include <audio_element.h>
#include <audio_pipeline.h>

#include "adf_audio_element.h"

namespace esphome {
namespace esp_adf {

enum PipelineState : uint8_t {
  STATE_UNAVAILABLE = 0,
  STATE_STOPPED,
  STATE_RUNNING,
  STATE_PAUSED,
};


class ADFPipeline {
public:
    void setup(){ build_adf_pipeline_(); }
    
    void append_component( ADFAudioComponent* ADFAudioElement );
    int get_number_of_components(){return this->audio_components_.size();}
    PipelineState getState(){ return this->state_; }
    void start();
    void stop();
    void pause();
    void resume();

    void init();
    void reset();

    void watch(){this->watch_();}

    ADFAudioComponent* get_first_component(){ return this->audio_components_[0]; }
    ADFAudioComponent* get_last_component (){ return this->audio_components_.back(); };

protected:
    void watch_();
    
    void build_adf_pipeline_();
    void terminate_pipeline_();
    
    void deinit_all_();
    void reset_states_();
    
    PipelineState state_{PipelineState::STATE_UNAVAILABLE};
    std::vector<ADFAudioComponent*> audio_components_;
    audio_pipeline_handle_t adf_pipeline_{};
    audio_event_iface_handle_t adf_pipeline_event_{};
    audio_element_handle_t adf_last_element_in_pipeline_{};
};

}
}

#endif