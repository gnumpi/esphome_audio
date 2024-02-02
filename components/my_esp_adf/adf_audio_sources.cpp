#include "adf_audio_sources.h"
#include "adf_pipeline.h"
#ifdef USE_ESP_IDF 

#include <http_stream.h>
#include <mp3_decoder.h>

namespace esphome {
namespace esp_adf {

void HTTPStreamReaderAndDecoder::init_adf_elements_(){
  if( sdk_audio_elements_.size() > 0 )
    return;
  
  http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
  http_cfg.task_core = 0;
  http_cfg.out_rb_size = 4 * 512; 
  http_stream_reader_ = http_stream_init(&http_cfg);
  
  audio_element_set_uri( this->http_stream_reader_, "https://dl.espressif.com/dl/audio/ff-16b-2c-44100hz.mp3");
  
  sdk_audio_elements_.push_back( this->http_stream_reader_ );
  sdk_element_tags_.push_back("http");
  
  mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
  mp3_cfg.out_rb_size = 12 * 512;
  decoder_ = mp3_decoder_init(&mp3_cfg);
  
  sdk_audio_elements_.push_back( this->decoder_ );
  sdk_element_tags_.push_back("decoder");
}

void HTTPStreamReaderAndDecoder::set_stream_uri(const char *uri){
  audio_element_set_uri(this->http_stream_reader_, uri);
}

void HTTPStreamReaderAndDecoder::sdk_event_handler_(audio_event_iface_msg_t &msg){
  
  audio_element_handle_t mp3_decoder = this->decoder_;
  if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
      && msg.source == (void *) mp3_decoder
      && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
      audio_element_info_t music_info = {0};
      audio_element_getinfo(mp3_decoder, &music_info);

      esph_log_i(get_name().c_str(), "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                music_info.sample_rates, music_info.bits, music_info.channels);
      
      AudioPipelineSettingsRequest request{this};
      request.sampling_rate = music_info.sample_rates;
      request.bit_depth = music_info.bits;
      request.number_of_channels = music_info.channels;
      if( !pipeline_->request_settings(request) ){
        pipeline_->on_settings_request_failed(request);
      }
  }
}


}
}

#endif