#include "adf_audio_element.h"
#ifdef USE_ESP_IDF 
namespace esphome {
namespace esp_adf {


std::string ADFAudioComponent::get_adf_element_tag(int element_indx ){
    if( element_indx >= 0 && element_indx < this->element_tags_.size() ){
        return this->element_tags_[element_indx];
    }
    return "Unknown";
}

void ADFAudioComponent::deinit_adf_elements_(){
  while( !this->adf_audio_elements_.empty() ){
    audio_element_deinit( this->adf_audio_elements_.back() );
    this->adf_audio_elements_.pop_back();
  }
  this->element_tags_.clear();
}

void ADFAudioComponent::adf_reset_states_(){
  for( auto el : this->adf_audio_elements_ ){
    audio_element_reset_state(el);
  }  
}


}
}


#endif