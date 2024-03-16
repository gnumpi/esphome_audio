#include "adf_audio_element.h"
#ifdef USE_ESP_IDF
namespace esphome {
namespace esp_adf {

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

}  // namespace esp_adf
}  // namespace esphome

#endif
