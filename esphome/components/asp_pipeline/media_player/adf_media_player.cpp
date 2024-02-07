#include "adf_media_player.h"
#include "esphome/core/log.h"
#include "mp3_decoder.h"
#include <filter_resample.h>

#ifdef USE_ESP_IDF 

namespace esphome {
namespace esp_adf {

static const char *const TAG = "adf_audio";

void ADFMediaPlayer::setup() {
  state = media_player::MEDIA_PLAYER_STATE_IDLE;
  pipeline.init();
}

void ADFMediaPlayer::dump_config() {
  esph_log_config(TAG, "ESP-ADF-MediaPlayer:");
  int components = pipeline.get_number_of_elements();
  esph_log_config(TAG, "  Number of ASPComponents: %d", components);
}


void ADFMediaPlayer::set_stream_uri(const char *uri){
  http_and_decoder_.set_stream_uri(uri);
}  


void ADFMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  if (call.get_media_url().has_value()) {
      current_url_ = call.get_media_url();

    if (state == media_player::MEDIA_PLAYER_STATE_PLAYING
        || state == media_player::MEDIA_PLAYER_STATE_PAUSED ) {
      pipeline.stop();
      pipeline.reset();
      set_stream_uri( current_url_.value().c_str() );
      pipeline.start();
    } else {
      pipeline.init();
      set_stream_uri( current_url_.value().c_str() );
      pipeline.start();
    }
  }
  
  if (call.get_volume().has_value()) {
    set_volume_(call.get_volume().value());
    unmute_();
  }
  
  if (call.get_command().has_value()) {
    switch (call.get_command().value()) {
      case media_player::MEDIA_PLAYER_COMMAND_PLAY:
        state = media_player::MEDIA_PLAYER_STATE_PLAYING;
        if( pipeline.getState() == PipelineState::STOPPED
             || pipeline.getState() == PipelineState::UNAVAILABLE
        ){
          pipeline.start();
        }
        else if( pipeline.getState() == PipelineState::PAUSED)
        {
          pipeline.resume();
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_PAUSE:
        state = media_player::MEDIA_PLAYER_STATE_PAUSED;
        if( pipeline.getState() == PipelineState::RUNNING)
        {
          pipeline.pause();
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
        if (pipeline.getState() == PipelineState::STOPPED ||
            pipeline.getState() == PipelineState::PAUSED
          ){
          state = media_player::MEDIA_PLAYER_STATE_PLAYING;
        } else {
          state = media_player::MEDIA_PLAYER_STATE_PAUSED;
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_UP: {
        float new_volume = this->volume + 0.1f;
        if (new_volume > 1.0f)
          new_volume = 1.0f;
        set_volume_(new_volume);
        unmute_();
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_DOWN: {
        float new_volume = this->volume - 0.1f;
        if (new_volume < 0.0f)
          new_volume = 0.0f;
        set_volume_(new_volume);
        unmute_();
        break;
      }
    }
  }
  publish_state();
}

media_player::MediaPlayerTraits ADFMediaPlayer::get_traits() {
  auto traits = media_player::MediaPlayerTraits();
  traits.set_supports_pause(true);
  return traits;
};


void ADFMediaPlayer::mute_(){
  AudioPipelineSettingsRequest request;
  request.mute = 1;
  if( pipeline.request_settings(request) )
  {
    muted_ = true;  
    publish_state();
  }
}

void ADFMediaPlayer::unmute_(){
  AudioPipelineSettingsRequest request;
  request.mute = 0;
  if( pipeline.request_settings(request) )
  {
    muted_ = false;  
    publish_state();
  }
}

void ADFMediaPlayer::set_volume_(float volume, bool publish){
  AudioPipelineSettingsRequest request;
  request.target_volume = volume;
  if( pipeline.request_settings(request) )
  {
    this->volume = volume;  
    if (publish) publish_state();
  }
}

void ADFMediaPlayer::on_pipeline_state_change(PipelineState state){
  esph_log_i(TAG, "got new pipeline state: %d", (int) state );     
  switch(state){
    case PipelineState::UNAVAILABLE:
    case PipelineState::STOPPED:
       this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
       publish_state();
       break;
    case PipelineState::PAUSED:
       this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
       publish_state();
       break;
    case PipelineState::RUNNING:
      this->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
      publish_state();
      break;
    default:
      break;
  }
}

}
}
#endif