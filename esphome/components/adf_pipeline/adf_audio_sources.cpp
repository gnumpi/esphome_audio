#include "adf_audio_sources.h"
#include "adf_pipeline.h"
#ifdef USE_ESP_IDF

#include <http_stream.h>
#include <mp3_decoder.h>
#include <raw_stream.h>

#include "sdk_ext.h"

namespace esphome {
namespace esp_adf {

static const char *const TAG = "esp_audio_sources";

/*
HTTPStreamReaderAndDecoder
*/


bool HTTPStreamReaderAndDecoder::init_adf_elements_() {
  if (sdk_audio_elements_.size() > 0)
    return true;

  http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
  http_cfg.task_core = 0;
  http_cfg.out_rb_size = 4 * 1024;
  http_stream_reader_ = http_stream_init(&http_cfg);
  http_stream_reader_->buf_size =  1024;
  audio_element_set_uri(this->http_stream_reader_, this->current_url_.c_str());

  sdk_audio_elements_.push_back(this->http_stream_reader_);
  sdk_element_tags_.push_back("http");

  mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
  mp3_cfg.out_rb_size = 4 * 1024;
  decoder_ = mp3_decoder_init(&mp3_cfg);

  sdk_audio_elements_.push_back(this->decoder_);
  sdk_element_tags_.push_back("decoder");
  this->element_state_ = PipelineElementState::INITIALIZED;
  return true;
}

void HTTPStreamReaderAndDecoder::clear_adf_elements_() {
  //make sure that deinit of pipeline was called first
  this->sdk_audio_elements_.clear();
  this->sdk_element_tags_.clear();
  this->element_state_ = PipelineElementState::UNINITIALIZED;
}

void HTTPStreamReaderAndDecoder::reset_() {
  this->element_state_ = PipelineElementState::INITIALIZED;
}

void HTTPStreamReaderAndDecoder::set_stream_uri(const std::string& new_url) {
  this->current_url_ = new_url;
}

void HTTPStreamReaderAndDecoder::prepare_elements(){
  this->element_state_ = PipelineElementState::PREPARE;
}

// called while pipeline is in PREPARING state
bool HTTPStreamReaderAndDecoder::is_ready(){
  switch(this->element_state_){
    case PipelineElementState::READY:
      return true;
    case PipelineElementState::PREPARE:
      this->element_state_ = PipelineElementState::PREPARING;
      audio_element_set_uri(this->http_stream_reader_, this->current_url_.c_str());
      this->start_prepare_pipeline_();
      return false;
    case PipelineElementState::WAIT_FOR_PREPARATION_DONE:
      return this->set_ready_when_prepare_pipeline_stopped_();
    default:
      return false;
  }
}

// Start pipeline elements necessary for receiving audio settings from stream
void HTTPStreamReaderAndDecoder::start_prepare_pipeline_(){
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
  esph_log_d(TAG, "Streamer status: %d", audio_element_get_state(this->http_stream_reader_) );
  esph_log_d(TAG, "decoder status: %d", audio_element_get_state(this->decoder_) );
}

void HTTPStreamReaderAndDecoder::terminate_prepare_pipeline_(){
  audio_element_stop(this->http_stream_reader_);
  audio_element_stop(this->decoder_);
  this->element_state_ = PipelineElementState::WAIT_FOR_PREPARATION_DONE;
}

bool HTTPStreamReaderAndDecoder::set_ready_when_prepare_pipeline_stopped_(){
  bool stopped = audio_element_wait_for_stop_ms(this->http_stream_reader_, 0) == ESP_OK;
  stopped = stopped &&  audio_element_wait_for_stop_ms(this->decoder_, 0) == ESP_OK;
  if( stopped ){
    audio_element_reset_state(this->http_stream_reader_);
    audio_element_reset_state(this->decoder_);
    audio_element_reset_input_ringbuf(this->decoder_);
    audio_element_reset_output_ringbuf(this->decoder_);
    this->element_state_ = PipelineElementState::READY;
  }
  return stopped;
}

//wait for audio information in stream and send new audio settings to pipeline
void HTTPStreamReaderAndDecoder::sdk_event_handler_(audio_event_iface_msg_t &msg) {
  audio_element_handle_t mp3_decoder = this->decoder_;
  if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) mp3_decoder &&
      msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
    audio_element_info_t music_info{};
    audio_element_getinfo(mp3_decoder, &music_info);

    esph_log_i(get_name().c_str(), "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
               music_info.sample_rates, music_info.bits, music_info.channels);

    AudioPipelineSettingsRequest request{this};
    request.sampling_rate = music_info.sample_rates;
    request.bit_depth = music_info.bits;
    request.number_of_channels = music_info.channels;
    if (!pipeline_->request_settings(request)) {
      esph_log_e(TAG, "Requested audio settings, didn't get accepted");
      pipeline_->on_settings_request_failed(request);
    }

    // necessary audio information has been received, terminate preparation pipeline
    if( this->element_state_ == PipelineElementState::PREPARING )
    {
      this->terminate_prepare_pipeline_();
    }
  }
}

/*
PCM SOURCE
*/

bool PCMSource::init_adf_elements_() {
  raw_stream_cfg_t raw_cfg = {
      .type = AUDIO_STREAM_WRITER,
      .out_rb_size = 8 * 1024,
  };

  adf_raw_stream_writer_ = raw_stream_init(&raw_cfg);
  audio_element_set_output_timeout(this->adf_raw_stream_writer_, 10 / portTICK_PERIOD_MS);
  this->sdk_audio_elements_.push_back(this->adf_raw_stream_writer_);
  this->sdk_element_tags_.push_back("pcm_writer");
  return true;
}

//int PCMSource::stream_write(char *buffer, int len) { return len ? raw_stream_write(adf_raw_stream_writer_, buffer, len) : 0; }


int PCMSource::stream_write(char *buffer, int len) {
  int ret = audio_element_output(this->adf_raw_stream_writer_, buffer, len);
  if (ret < 0 && (ret != AEL_IO_TIMEOUT)) {
    audio_element_report_status(this->adf_raw_stream_writer_, AEL_STATUS_STATE_STOPPED);
  } else if (ret < 0) {
    return 0;
  }
  return ret;
}


bool PCMSource::has_buffered_data() const {
  ringbuf_handle_t rb = audio_element_get_output_ringbuf(adf_raw_stream_writer_);
  return rb_bytes_filled(rb) > 0;
}

}  // namespace esp_adf
}  // namespace esphome

#endif
