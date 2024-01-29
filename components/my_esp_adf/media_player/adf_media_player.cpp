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


void ADFMediaPlayer::add_to_pipeline( ADFAudioComponent* component ){
   if( this->pipeline.get_number_of_components() == 0 ){
      this->pipeline.append_component(this);
   }
   this->pipeline.append_component(component);
}

void ADFMediaPlayer::init_adf_elements_(){
  if( this->adf_audio_elements_.size() > 0 )
    return;
  
  http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
  http_cfg.task_core = 0;
  http_cfg.out_rb_size = 4 * 512; 
  this->http_stream_reader_ = http_stream_init(&http_cfg);
  
  audio_element_set_uri( this->http_stream_reader_, "https://dl.espressif.com/dl/audio/ff-16b-2c-44100hz.mp3");
  
  this->adf_audio_elements_.push_back( this->http_stream_reader_ );
  this->element_tags_.push_back("http");
  
  
  mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
  mp3_cfg.out_rb_size = 12 * 512;
  this->decoder_ = mp3_decoder_init(&mp3_cfg);
  
  this->adf_audio_elements_.push_back( this->decoder_ );
  this->element_tags_.push_back("decoder");

  rsp_filter_cfg_t rsp_cfg = {
      .src_rate = 44100,
      .src_ch = 2,
      .dest_rate = 16000,
      .dest_bits = 16,
      .dest_ch = 2,
      .src_bits = 16,
      .mode = RESAMPLE_ENCODE_MODE,
      .max_indata_bytes = 12 * 512,
      .out_len_bytes = 4 * 512,
      .type = ESP_RESAMPLE_TYPE_RESAMPLE,
      .complexity = 2,
      .down_ch_idx = 0,
      .prefer_flag = ESP_RSP_PREFER_TYPE_SPEED,
      .out_rb_size = RSP_FILTER_RINGBUFFER_SIZE,
      .task_stack = 100 * 1024,
      .task_core = RSP_FILTER_TASK_CORE,
      .task_prio = RSP_FILTER_TASK_PRIO,
      .stack_in_ext = false,
  };
  audio_element_handle_t filter = rsp_filter_init(&rsp_cfg);
  
  
  //this->adf_audio_elements_.push_back( filter );
  //this->element_tags_.push_back("resample");

  
}

void ADFMediaPlayer::set_stream_uri(const char *uri){
  
  audio_element_set_uri(this->http_stream_reader_, uri);
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
  /*
  if (this->i2s_state_ != I2S_STATE_RUNNING) {
    return;
  }
  */

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
   ADFAudioComponent* last = pipeline.get_last_component();
   last->mute(); 
   this->muted_ = last->is_muted();  
   publish_state();
}

void ADFMediaPlayer::unmute_(){
   ADFAudioComponent* last = pipeline.get_last_component();
   last->unmute(); 
   this->muted_ = last->is_muted();  
   publish_state();
}

void ADFMediaPlayer::set_volume_(float volume, bool publish){
   ADFAudioComponent* last = pipeline.get_last_component();
   last->set_volume( volume );
   this->volume = last->get_volume();
   if( publish ) publish_state();
}

}
}
#endif