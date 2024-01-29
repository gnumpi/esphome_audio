#pragma once

#ifdef USE_ESP_IDF 

#include "../adf_audio_element.h"

namespace esphome {
namespace esp_adf {


class AudioOutTraits {
 public:
  AudioOutTraits() = default;

  void set_supported_frequencies() {}
  void set_supported_bit_depths() {}
  void set_suppports_muting(bool muting_support) {}
  void set_suppports_volume_control(bool volume_control_support) {}
  void set_supported_output_selections() {}

 protected:
};




class ADFAudioOut : public ADFAudioElement {
public:
  
  
  
  void mute();
  void unmute();
  void set_volume(uint8_t volume_percentage);
  void set_output(uint8_t output_selector);
  uint8_t get_volume();



};


}
}
#endif