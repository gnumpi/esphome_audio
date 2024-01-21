#pragma once

#ifdef USE_ESP_IDF 

#include "esphome/components/media_player/media_player.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/helpers.h"

#include "audio_pipeline.h"
#include "audio_element.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "http_stream.h"

namespace esphome {
namespace esp_adf {

class ADFMediaPlayer : public Component, public media_player::MediaPlayer {
 public:
  
  // Component implementations
  void setup() override;
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }
  
  void loop() override;
  void dump_config() override;

  // MediaPlayer implementations
  bool is_muted() const override { return this->muted_; }
  media_player::MediaPlayerTraits get_traits() override;
  
  void setup_pipeline();
  void setup_decoder();
  void set_output_stream(audio_element_handle_t stream_writer);
  void set_stream_uri(const char *uri);
   

  void start();
  void stop();

 protected:
  // MediaPlayer implementation
  void control(const media_player::MediaPlayerCall &call) override;

  void mute_();
  void unmute_();
  void set_volume_(float volume, bool publish = true);

  void pipeline_event_handler(audio_event_iface_msg_t& msg); 
  
  void start_();
  void stop_();
  void play_();
  void loop_();

  audio_event_iface_handle_t evt_;
  audio_pipeline_handle_t pipeline_;
  audio_element_handle_t http_stream_reader_;
  audio_element_handle_t decoder_;
  audio_element_handle_t stream_writer_ = {nullptr};
  
  uint8_t dout_pin_{0};

  bool muted_{false};
  float unmuted_volume_{0};

  HighFrequencyLoopRequester high_freq_;
  optional<std::string> current_url_{};
};



}
}

#endif