#pragma once

#ifdef USE_ESP_IDF 
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include <audio_element.h>
#include <audio_pipeline.h>
#include <vector>

namespace esphome {
namespace esp_adf {


class ADFAudioComponent {
public:
  //void setup() override {};
  //void dump_config() override {};
  //void loop() override {};

  std::vector<audio_element_handle_t> get_adf_elements(){ return this->adf_audio_elements_; }
  std::string get_adf_element_tag(int element_indx );
  void init_adf_elements() {this->init_adf_elements_(); }
  void deinit_adf_elements(){}
  
  
  virtual bool hasInputBuffer()   const {return false;}
  virtual bool hasOutputBuffer()  const {return false;}
  virtual bool hasVolumeControl() const {return false;}
  virtual bool isMuteable()       const {return false;}

  virtual void mute(){}
  virtual void unmute(){}
  virtual bool is_muted() const {return false;}
  virtual void set_volume(float volume){}
  virtual float get_volume() {return 1.; }

  virtual void set_sampling_frequency(int freq){}
  virtual void set_bit_depth(int bits){}
  virtual void set_number_of_channels(int channels){}
          
  
  
  void adf_reset_states_();
protected:
  
  virtual void init_adf_elements_() = 0;
  virtual void deinit_adf_elements_();

  std::vector<audio_element_handle_t> adf_audio_elements_;
  std::vector<std::string> element_tags_;
};


}
}
#endif