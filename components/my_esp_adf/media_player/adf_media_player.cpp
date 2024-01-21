#include "adf_media_player.h"
#include "esphome/core/log.h"
#include "mp3_decoder.h"

#ifdef USE_ESP_IDF 

namespace esphome {
namespace esp_adf {

static const char *const TAG = "adf_audio";

void ADFMediaPlayer::setup() {
  ESP_LOGCONFIG(TAG, "Setting up MediaPlayer...");
  this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
}

void ADFMediaPlayer::setup_decoder(){
  ESP_LOGI(TAG, "[2.3] Create mp3 decoder to decode mp3 file");
  mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
  this->decoder_ = mp3_decoder_init(&mp3_cfg);
}

void ADFMediaPlayer::set_output_stream(audio_element_handle_t stream_writer){
  this->stream_writer_ = stream_writer;
}

void ADFMediaPlayer::setup_pipeline(){ 
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  this->pipeline_ = audio_pipeline_init(&pipeline_cfg);
  mem_assert(this->pipeline_);
  
  http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
  this->http_stream_reader_ = http_stream_init(&http_cfg);

  assert( this->decoder_ != NULL );
  assert( this->stream_writer_ != NULL );
    
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  this->evt_ = audio_event_iface_init(&evt_cfg);
  
  audio_pipeline_register(this->pipeline_, this->http_stream_reader_, "http");
  audio_pipeline_register(this->pipeline_, this->decoder_,            "dec");
  audio_pipeline_register(this->pipeline_, this->stream_writer_,      "out");  

  const char *link_tag[3] = {"http", "dec", "out"};
  audio_pipeline_link(this->pipeline_, &link_tag[0], 3);

  audio_pipeline_set_listener(this->pipeline_, this->evt_);
}

void ADFMediaPlayer::set_stream_uri(const char *uri){
  audio_element_set_uri(this->http_stream_reader_, uri);
}  


void ADFMediaPlayer::pipeline_event_handler(audio_event_iface_msg_t &msg) {
    
    /* Received music info report from decoder element*/ 
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void *) this->decoder_
        && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
        audio_element_info_t music_info = {0};
        audio_element_getinfo(this->decoder_, &music_info);

        ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                    music_info.sample_rates, music_info.bits, music_info.channels);

        //i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
        return;
    }

    /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) this->stream_writer_
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
        ESP_LOGW(TAG, "[ * ] Stop event received");
        return;
    }
}

void ADFMediaPlayer::control(const media_player::MediaPlayerCall &call) {
    if (call.get_media_url().has_value()) {
    this->current_url_ = call.get_media_url();

    if (this->state == media_player::MEDIA_PLAYER_STATE_PLAYING) {
      this->set_stream_uri( this->current_url_.value().c_str() );

    } else {
      this->start();
    }
  }
}


}
}
#endif