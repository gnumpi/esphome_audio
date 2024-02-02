#include "adf_media_player.h"
#include "esphome/core/log.h"
#include "mp3_decoder.h"
#include <filter_resample.h>

#ifdef USE_ESP_IDF 

namespace esphome {
namespace esp_adf {

static const char *const TAG = "adf_audio";

void ADFMediaPlayer::setup() {
  this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
  this->pipeline.setup();
}

void ADFMediaPlayer::dump_config() {
  esph_log_config(TAG, "ESP-ADF-MediaPlayer:");
  int components = this->pipeline.get_number_of_components();
  esph_log_config(TAG, "  Number of ASPComponents: %d", components);
}

void ADFMediaPlayer::loop() {
  this->pipeline.watch();
}

void ADFMediaPlayer::set_stream_uri(const char *uri){
  this->http_and_decoder_.set_stream_uri(uri);
}  

void ADFMediaPlayer::pipeline_event_handler(audio_event_iface_msg_t &msg) {
    
}

void ADFMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  if (call.get_media_url().has_value()) {
      this->current_url_ = call.get_media_url();

    if (this->state == media_player::MEDIA_PLAYER_STATE_PLAYING
        || this->state == media_player::MEDIA_PLAYER_STATE_PAUSED ) {
      this->pipeline.stop();
      this->pipeline.reset();
      this->set_stream_uri( this->current_url_.value().c_str() );
      this->pipeline.start();
    } else {
      this->pipeline.init();
      this->set_stream_uri( this->current_url_.value().c_str() );
      this->start();
    }
  }
  
  if (call.get_volume().has_value()) {
    this->volume = call.get_volume().value();
    this->set_volume_(volume);
    this->unmute_();
  }
  
  if (call.get_command().has_value()) {
    switch (call.get_command().value()) {
      case media_player::MEDIA_PLAYER_COMMAND_PLAY:
        this->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
        if( this->pipeline.getState() == PipelineState::STATE_STOPPED
             || this->pipeline.getState() == PipelineState::STATE_UNAVAILABLE
        ){
          this->pipeline.start();
        }
        else if( this->pipeline.getState() == PipelineState::STATE_PAUSED)
        {
          this->pipeline.resume();
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_PAUSE:
        this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
        if( this->pipeline.getState() == PipelineState::STATE_RUNNING)
        {
          this->pipeline.pause();
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_STOP:
        this->stop();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_MUTE:
        this->mute_();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_UNMUTE:
        this->unmute_();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_TOGGLE:
        if (this->pipeline.getState() == PipelineState::STATE_STOPPED ||
            this->pipeline.getState() == PipelineState::STATE_PAUSED
          ){
          this->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
        } else {
          this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_UP: {
        float new_volume = this->volume + 0.1f;
        if (new_volume > 1.0f)
          new_volume = 1.0f;
        this->set_volume_(new_volume);
        this->unmute_();
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_DOWN: {
        float new_volume = this->volume - 0.1f;
        if (new_volume < 0.0f)
          new_volume = 0.0f;
        this->set_volume_(new_volume);
        this->unmute_();
        break;
      }
    }
  }
  this->publish_state();
}

media_player::MediaPlayerTraits ADFMediaPlayer::get_traits() {
  auto traits = media_player::MediaPlayerTraits();
  traits.set_supports_pause(true);
  return traits;
};


void ADFMediaPlayer::mute_(){
   ADFPipelineElement* last = pipeline.get_last_component();
   last->mute(); 
   this->muted_ = last->is_muted();  
   publish_state();
}

void ADFMediaPlayer::unmute_(){
   ADFPipelineElement* last = pipeline.get_last_component();
   last->unmute(); 
   this->muted_ = last->is_muted();  
   publish_state();
}

void ADFMediaPlayer::set_volume_(float volume, bool publish){
   ADFPipelineElement* last = pipeline.get_last_component();
   last->set_volume( volume );
   this->volume = last->get_volume();
   if( publish ) publish_state();
}

}
}
#endif