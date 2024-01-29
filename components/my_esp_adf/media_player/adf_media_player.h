#pragma once

#ifdef USE_ESP_IDF 

#include "esphome/components/media_player/media_player.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/helpers.h"

#include "../adf_audio_element.h"
#include "../adf_pipeline.h"

#include "audio_pipeline.h"
#include "audio_element.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "http_stream.h"

#include <raw_stream.h>

namespace esphome {
namespace esp_adf {

class ADFMediaPlayer : public ADFAudioComponent, public media_player::MediaPlayer, public Component {
 public:
  
  // Component implementations
  void setup() override;
  void add_to_pipeline( ADFAudioComponent* component );
  
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }
  
  void loop() override;
  void dump_config() override;

  // MediaPlayer implementations
  bool is_muted() const override { return this->muted_; }
  media_player::MediaPlayerTraits get_traits() override;
  
  void set_stream_uri(const char *uri);
   
  void start(){}
  void stop(){}
  
  // ADFAudioComponent implementations
  bool hasInputBuffer()  const override {return false;}
  bool hasOutputBuffer() const override {return true; }

 protected:
  // MediaPlayer implementation
  void control(const media_player::MediaPlayerCall &call) override;

  void mute_();
  void unmute_();
  
  void set_volume_(float volume, bool publish = true);

  void pipeline_event_handler(audio_event_iface_msg_t& msg); 
  
  void init_adf_elements_();

  audio_element_handle_t http_stream_reader_{};
  audio_element_handle_t decoder_{};
  ADFPipeline pipeline;    
  
  bool muted_{false};
  float unmuted_volume_{0};
  optional<std::string> current_url_{};
};



}
}

#endif