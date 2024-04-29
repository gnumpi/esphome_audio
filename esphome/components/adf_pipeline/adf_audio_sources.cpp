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
  mp3_cfg.task_prio = 2;
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
  esph_log_d(TAG, "Set new uri: %s", new_url.c_str());
}

bool HTTPStreamReaderAndDecoder::prepare_elements(bool initial_call){
  if (initial_call) {
    esph_log_d(TAG, "Prepare elements called!");
    this->element_state_ = PipelineElementState::PREPARE;
    this->desired_state_ = PipelineElementState::READY;
    this->audio_settings_reported_ = false;
  }
  else if ( this->element_state_ == PipelineElementState::READY ){
    return true;
  }
  this->preparing_step();
  return false;
}

bool HTTPStreamReaderAndDecoder::pause_elements(bool initial_call){
  if ( this->element_state_ == PipelineElementState::PAUSED ){
    return true;
  }
  this->desired_state_ = PipelineElementState::PAUSED;
  ADFPipelineElement::pause_elements(true);
  this->preparing_step();
  return true;
}


bool HTTPStreamReaderAndDecoder::preparing_step(){
  //esph_log_d(TAG, "Called pre-step in state: %d", (int) this->element_state_);
  switch(this->element_state_){
    case PipelineElementState::READY:
      break;

    case PipelineElementState::WAITING_FOR_SDK_EVENT:
      //check for timeout
      break;

    case PipelineElementState::PREPARE:
      esph_log_d(TAG, "Use fixed settings: %s", this->fixed_settings_ ? "yes" : "no");
      esph_log_d(TAG, "Streamer status: %d", audio_element_get_state(this->http_stream_reader_) );
      esph_log_d(TAG, "decoder status: %d", audio_element_get_state(this->decoder_) );

      audio_element_set_uri(this->http_stream_reader_, this->current_url_.c_str());

      if( this->fixed_settings_ ){
        AudioPipelineSettingsRequest request{this};
        request.sampling_rate = 16000;
        request.bit_depth = 16;
        request.number_of_channels = 1;
        if (!pipeline_->request_settings(request)) {
          esph_log_e(TAG, "Requested audio settings, didn't get accepted");
          pipeline_->on_settings_request_failed(request);
          this->element_state_ = PipelineElementState::ERROR;
        }
        else{
          this->audio_settings_reported_ = true;
        }
      }

      ADFPipelineElement::prepare_elements(true);
      if( this->element_state_ != PipelineElementState::ERROR)
      {
        this->element_state_ = PipelineElementState::PREPARING;
      }
      break;

    case PipelineElementState::PREPARING:
      if( ADFPipelineElement::prepare_elements(false))
      {
        this->element_state_ = PipelineElementState::PAUSED;
      }
      break;


    case PipelineElementState::PAUSING:
      if( ADFPipelineElement::pause_elements(false))
      {
        this->element_state_ = PipelineElementState::PAUSED;
      }
      break;
    /*
      esph_log_d(TAG, "Waiting for Pausing." );
      esph_log_d(TAG, "Streamer status: %d", audio_element_get_state(this->http_stream_reader_) );
      esph_log_d(TAG, "decoder status: %d", audio_element_get_state(this->decoder_) );
      if(    audio_element_get_state(this->http_stream_reader_) == AEL_STATE_PAUSED
          && audio_element_get_state(this->decoder_) == AEL_STATE_PAUSED
        ) this->element_state_ = PipelineElementState::PAUSED;
      break;
    */
    case PipelineElementState::PAUSED:
      if( this->desired_state_ == PipelineElementState::READY )
      {
        //audio_element_reset_input_ringbuf(this->decoder_);
        //audio_element_reset_output_ringbuf(this->decoder_);
        if( !this->audio_settings_reported_ ){
          ADFPipelineElement::resume_elements(true);
          if( this->element_state_ != PipelineElementState::ERROR){
            this->element_state_ = PipelineElementState::RESUMING;
          }
        }
        else {
          http_stream_restart(this->http_stream_reader_);
          audio_event_iface_discard(this->decoder_->iface_event);
          this->element_state_ = PipelineElementState::READY;
        }
      } else if (this->desired_state_ == PipelineElementState::STOPPED )
      {
        this->element_state_ = PipelineElementState::STOPPING;
      }
      break;

    case PipelineElementState::RESUMING:
      if( ADFPipelineElement::resume_elements(false))
      {
        this->element_state_ = PipelineElementState::WAITING_FOR_SDK_EVENT;
      }
      break;


    case PipelineElementState::STOPPING:
      /*
      esph_log_d(TAG, "Waiting for Stopping." );
      esph_log_d(TAG, "Streamer status: %d", audio_element_get_state(this->http_stream_reader_) );
      esph_log_d(TAG, "decoder status: %d", audio_element_get_state(this->decoder_) );
      if(    audio_element_get_state(this->http_stream_reader_) == AEL_STATE_STOPPED
          && audio_element_get_state(this->decoder_) == AEL_STATE_STOPPED
        ) this->element_state_ = PipelineElementState::STOPPED;
      */
      break;
    case PipelineElementState::ERROR:
      return false;
      break;
    default:
      break;
  }
  return true;
}

/*
// called while pipeline is in PREPARING state
bool HTTPStreamReaderAndDecoder::is_ready(){
  return this->element_state_ == PipelineElementState::READY;
}
*/

// called while pipeline is in STOPPING state
bool HTTPStreamReaderAndDecoder::ready_to_stop(){
  switch(this->element_state_){
    case PipelineElementState::PREPARING:
    case PipelineElementState::WAITING_FOR_SDK_EVENT:
      this->pause_sdk_elements_();
      this->element_state_ = PipelineElementState::PAUSING;
      return false;

    case PipelineElementState::PAUSING:
    case PipelineElementState::STOPPING:
      return this->set_ready_when_prepare_pipeline_paused_();

    case PipelineElementState::ERROR:
      return true;
    default:
      return true;
  }
}

/*
//Elements are in PAUSE state after calling audio_element_run
void HTTPStreamReaderAndDecoder::start_sdk_tasks_(){
  if( audio_element_run(this->http_stream_reader_) != ESP_OK )
  {
    esph_log_e(TAG, "Starting http streamer task failed");
    this->element_state_ = PipelineElementState::ERROR;
  }
  audio_event_iface_discard(this->decoder_->iface_event);
  if( audio_element_run(this->decoder_) != ESP_OK ){
    esph_log_e(TAG, "Starting decoder streamer task failed");
    this->element_state_ = PipelineElementState::ERROR;
  }
  if( audio_element_pause(this->http_stream_reader_) != ESP_OK )
  {
    esph_log_e(TAG, "Pausing http streamer failed");
    this->element_state_ = PipelineElementState::ERROR;
  }
  if( audio_element_pause(this->decoder_) != ESP_OK ){
    esph_log_e(TAG, "Pausing decoder streamer failed");
    this->element_state_ = PipelineElementState::ERROR;
  }
}
*/

void HTTPStreamReaderAndDecoder::resume_sdk_elements_(){
  audio_element_reset_input_ringbuf(this->decoder_);
  audio_element_reset_output_ringbuf(this->decoder_);
  if( audio_element_resume(this->http_stream_reader_, 0, 20 / portTICK_RATE_MS) != ESP_OK)
  {
    //esph_log_e(TAG, "Resuming http streamer failed");
    //this->element_state_ = PipelineElementState::ERROR;
  }
  if( audio_element_resume(this->decoder_, 0, 20 / portTICK_RATE_MS) != ESP_OK ){
    //esph_log_e(TAG, "Resuming decoder failed");
    //this->element_state_ = PipelineElementState::ERROR;
  }
}


void HTTPStreamReaderAndDecoder::pause_sdk_elements_(){
  audio_element_abort_input_ringbuf(this->decoder_);
  audio_element_abort_output_ringbuf(this->decoder_);
  esp_err_t ret = audio_element_pause(this->http_stream_reader_);
  if( ret != ESP_OK ){
    esph_log_d(TAG, "Failed setting streamer to pause: %d", ret);
    this->element_state_ = PipelineElementState::ERROR;
  }
  //audio_element_reset_state(this->decoder_);
  ret = audio_element_pause(this->decoder_);
  if( ret != ESP_OK ){
    esph_log_d(TAG, "Failed setting decoder to pause: %d", ret);
    this->element_state_ = PipelineElementState::ERROR;
  }
}


// Start pipeline elements necessary for receiving audio settings from stream
void HTTPStreamReaderAndDecoder::start_prepare_pipeline_(){
  audio_element_reset_input_ringbuf(this->decoder_);
  audio_element_reset_output_ringbuf(this->decoder_);
  if( audio_element_run(this->http_stream_reader_) != ESP_OK )
  {
    esph_log_e(TAG, "Starting http streamer failed");
    this->element_state_ = PipelineElementState::ERROR;
  }
  audio_event_iface_discard(this->decoder_->iface_event);
  if( audio_element_run(this->decoder_) != ESP_OK ){
    esph_log_e(TAG, "Starting decoder streamer failed");
    this->element_state_ = PipelineElementState::ERROR;
  }
  if( audio_element_resume(this->http_stream_reader_, 0, 20 / portTICK_RATE_MS) != ESP_OK)
  {
    //esph_log_e(TAG, "Resuming http streamer failed");
    //this->element_state_ = PipelineElementState::ERROR;
  }
  if( audio_element_resume(this->decoder_, 0, 20 / portTICK_RATE_MS) != ESP_OK ){
    //esph_log_e(TAG, "Resuming decoder failed");
    //this->element_state_ = PipelineElementState::ERROR;
  }
  esph_log_d(TAG, "Streamer status: %d", audio_element_get_state(this->http_stream_reader_) );
  esph_log_d(TAG, "decoder status: %d", audio_element_get_state(this->decoder_) );
}



bool HTTPStreamReaderAndDecoder::set_ready_when_prepare_pipeline_paused_(){
  bool paused = audio_element_get_state(this->http_stream_reader_) == AEL_STATE_PAUSED && audio_element_get_state(this->decoder_) == AEL_STATE_PAUSED;
  if(paused){
    audio_element_reset_input_ringbuf(this->decoder_);
    audio_element_reset_output_ringbuf(this->decoder_);
    http_stream_restart(this->http_stream_reader_);
    this->element_state_ = PipelineElementState::READY;
  }
  return paused;
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

    // necessary audio information has been received, terminate preparation pipeline
    if( this->element_state_ == PipelineElementState::WAITING_FOR_SDK_EVENT )
    {
      AudioPipelineSettingsRequest request{this};
      request.sampling_rate = music_info.sample_rates;
      request.bit_depth = music_info.bits;
      request.number_of_channels = music_info.channels;
      if (!pipeline_->request_settings(request)) {
        esph_log_e(TAG, "Requested audio settings, didn't get accepted");
        pipeline_->on_settings_request_failed(request);
        this->element_state_ = PipelineElementState::ERROR;
      }
      else
      {
        this->audio_settings_reported_ = true;
        ADFPipelineElement::pause_elements(true);
        if( this->element_state_ != PipelineElementState::ERROR ){
          this->element_state_ = PipelineElementState::PAUSING;
        }
      }
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
