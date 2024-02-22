#include "adf_audio_sources.h"
#include "adf_pipeline.h"
#ifdef USE_ESP_IDF

#include <http_stream.h>
#include <mp3_decoder.h>
#include <raw_stream.h>

namespace esphome {
namespace esp_adf {

static const char *const TAG = "esp_audio_sources";

void HTTPStreamReaderAndDecoder::init_adf_elements_() {
  if (sdk_audio_elements_.size() > 0)
    return;

  http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
  http_cfg.task_core = 0;
  http_cfg.out_rb_size = 8 * 512;
  http_stream_reader_ = http_stream_init(&http_cfg);

  audio_element_set_uri(this->http_stream_reader_, "https://dl.espressif.com/dl/audio/ff-16b-2c-44100hz.mp3");

  sdk_audio_elements_.push_back(this->http_stream_reader_);
  sdk_element_tags_.push_back("http");

  mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
  mp3_cfg.out_rb_size = 8 * 512;
  decoder_ = mp3_decoder_init(&mp3_cfg);

  sdk_audio_elements_.push_back(this->decoder_);
  sdk_element_tags_.push_back("decoder");
}

void HTTPStreamReaderAndDecoder::set_stream_uri(const char *uri) {
  audio_element_set_uri(this->http_stream_reader_, uri);
  this->waiting_for_cfg_ = true;
  this->start_config_pipeline_();
}

void HTTPStreamReaderAndDecoder::start_config_pipeline_(){
  if( audio_element_run(this->http_stream_reader_) != ESP_OK )
  {
    esph_log_e(TAG, "Starting http streamer failed");
  }
  if( audio_element_run(this->decoder_) != ESP_OK ){
    esph_log_e(TAG, "Starting decoder streamer failed");
  }

  if( audio_element_resume(this->http_stream_reader_, 0, 2000 / portTICK_RATE_MS) != ESP_OK)
  {
    esph_log_e(TAG, "Resuming http streamer failed");
  }
  if( audio_element_resume(this->decoder_, 0, 2000 / portTICK_RATE_MS) != ESP_OK ){
    esph_log_e(TAG, "Resuming decoder failed");
  }
  esph_log_i(TAG, "Streamer status: %d", audio_element_get_state(this->http_stream_reader_) );
  esph_log_i(TAG, "decoder status: %d", audio_element_get_state(this->decoder_) );
}

void HTTPStreamReaderAndDecoder::terminate_config_pipeline_(){
  audio_element_stop(this->http_stream_reader_);
  audio_element_stop(this->decoder_);
  audio_element_reset_input_ringbuf(this->decoder_);
  audio_element_reset_output_ringbuf(this->decoder_);
  if( audio_element_wait_for_stop_ms(this->http_stream_reader_, portMAX_DELAY) == ESP_ERR_TIMEOUT )
  {
    esph_log_e(TAG, "Timeout while stopping stream reader!");
  }
  if(audio_element_wait_for_stop_ms(this->decoder_, portMAX_DELAY) == ESP_ERR_TIMEOUT){
    esph_log_e(TAG, "Timeout while stopping decoder!");
  }
  audio_element_reset_state(this->http_stream_reader_);
  audio_element_reset_state(this->decoder_);
}

bool HTTPStreamReaderAndDecoder::isReady(){
  return !this->waiting_for_cfg_;
}

void HTTPStreamReaderAndDecoder::sdk_event_handler_(audio_event_iface_msg_t &msg) {
  audio_element_handle_t mp3_decoder = this->decoder_;
  if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) mp3_decoder &&
      msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
    audio_element_info_t music_info = {0};
    audio_element_getinfo(mp3_decoder, &music_info);

    esph_log_i(get_name().c_str(), "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
               music_info.sample_rates, music_info.bits, music_info.channels);

    AudioPipelineSettingsRequest request{this};
    request.sampling_rate = music_info.sample_rates;
    request.bit_depth = music_info.bits;
    request.number_of_channels = music_info.channels;
    if (!pipeline_->request_settings(request)) {
      pipeline_->on_settings_request_failed(request);
    }
    if( this->waiting_for_cfg_)
    {
      this->terminate_config_pipeline_();
      this->waiting_for_cfg_ = false;
    }

  }
}

void PCMSource::init_adf_elements_() {
  raw_stream_cfg_t raw_cfg = {
      .type = AUDIO_STREAM_WRITER,
      .out_rb_size = 8 * 1024,
  };
  adf_raw_stream_writer_ = raw_stream_init(&raw_cfg);
  this->sdk_audio_elements_.push_back(this->adf_raw_stream_writer_);
  this->sdk_element_tags_.push_back("pcm_writer");
}

int PCMSource::stream_write(char *buffer, int len) { return raw_stream_write(adf_raw_stream_writer_, buffer, len); }

bool PCMSource::has_buffered_data() const {
  ringbuf_handle_t rb = audio_element_get_output_ringbuf(adf_raw_stream_writer_);
  return rb_bytes_filled(rb) > 0;
}

}  // namespace esp_adf
}  // namespace esphome

#endif
