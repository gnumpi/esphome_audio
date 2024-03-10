#include "adf_media_player.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF

namespace esphome {
namespace esp_adf {

static const char *const TAG = "adf_media_player";

void ADFMediaPlayer::setup() {
  state = media_player::MEDIA_PLAYER_STATE_IDLE;
}

void ADFMediaPlayer::dump_config() {
  esph_log_config(TAG, "ESP-ADF-MediaPlayer:");
  int components = pipeline.get_number_of_elements();
  esph_log_config(TAG, "  Number of ASPComponents: %d", components);
}

void ADFMediaPlayer::set_stream_uri(const std::string& new_uri) {
  this->current_uri_ = new_uri;
  http_and_decoder_.set_stream_uri(new_uri);
}

void ADFMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  if (call.get_media_url().has_value()) {
    set_stream_uri( call.get_media_url().value()) ;

    esph_log_d(TAG, "Got control call in state %d", state );
    if (state == media_player::MEDIA_PLAYER_STATE_PLAYING || state == media_player::MEDIA_PLAYER_STATE_PAUSED) {
      this->play_intent_ = true;
      pipeline.stop();
      return;
    } else {
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
        if (pipeline.getState() == PipelineState::STOPPED || pipeline.getState() == PipelineState::UNINITIALIZED) {
          pipeline.start();
        } else if (pipeline.getState() == PipelineState::PAUSED) {
          pipeline.resume();
        }
        else if (state == media_player::MEDIA_PLAYER_STATE_PLAYING || state == media_player::MEDIA_PLAYER_STATE_PAUSED){
          this->play_intent_ = true;
          pipeline.stop();
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_PAUSE:
        if (pipeline.getState() == PipelineState::RUNNING) {
          pipeline.pause();
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_STOP:
        pipeline.stop();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_MUTE:
        this->mute_();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_UNMUTE:
        this->unmute_();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_TOGGLE:
        if (pipeline.getState() == PipelineState::STOPPED || pipeline.getState() == PipelineState::PAUSED) {
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
}

// pausing is only supported if destroy_pipeline_on_stop is disabled
media_player::MediaPlayerTraits ADFMediaPlayer::get_traits() {
  auto traits = media_player::MediaPlayerTraits();
  traits.set_supports_pause( false );
  return traits;
};

void ADFMediaPlayer::mute_() {
  AudioPipelineSettingsRequest request;
  request.mute = 1;
  if (pipeline.request_settings(request)) {
    muted_ = true;
    publish_state();
  }
}

void ADFMediaPlayer::unmute_() {
  AudioPipelineSettingsRequest request;
  request.mute = 0;
  if (pipeline.request_settings(request)) {
    muted_ = false;
    publish_state();
  }
}

void ADFMediaPlayer::set_volume_(float volume, bool publish) {
  AudioPipelineSettingsRequest request;
  request.target_volume = volume;
  if (pipeline.request_settings(request)) {
    this->volume = volume;
    if (publish)
      publish_state();
  }
}

void ADFMediaPlayer::on_pipeline_state_change(PipelineState state) {
  esph_log_i(TAG, "got new pipeline state: %d", (int) state);
  switch (state) {
    case PipelineState::UNINITIALIZED:
    case PipelineState::STOPPED:
      this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
      publish_state();
      if( this->play_intent_ )
      {
        pipeline.start();
        this->play_intent_ = false;
      }
      break;
    case PipelineState::PAUSED:
      this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
      publish_state();
      break;
    case PipelineState::PREPARING:
    case PipelineState::STARTING:
    case PipelineState::RUNNING:
      this->set_volume_( this->volume, false);
      this->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
      publish_state();
      break;
    default:
      break;
  }
}

}  // namespace esp_adf
}  // namespace esphome
#endif
