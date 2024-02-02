#include "adf_audio_element.h"
#ifdef USE_ESP_IDF 
namespace esphome {
namespace esp_adf {


std::string ADFPipelineElement::get_adf_element_tag(int element_indx ){
    if( element_indx >= 0 && element_indx < this->sdk_element_tags_.size() ){
        return this->sdk_element_tags_[element_indx];
    }
    return "Unknown";
}

void ADFPipelineElement::deinit_adf_elements_(){
  while( !this->sdk_audio_elements_.empty() ){
    audio_element_deinit( this->sdk_audio_elements_.back() );
    this->sdk_audio_elements_.pop_back();
  }
  this->sdk_element_tags_.clear();
}

void ADFPipelineElement::adf_reset_states_(){
  for( auto el : this->sdk_audio_elements_ ){
    audio_element_reset_state(el);
  }  
}



}
}


#endif