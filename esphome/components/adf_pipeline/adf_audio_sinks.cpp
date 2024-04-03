#include "adf_audio_sinks.h"
#include "adf_pipeline.h"

#ifdef USE_ESP_IDF

#include <raw_stream.h>

namespace esphome {
namespace esp_adf {

static const char *const TAG = "esp_audio_sinks";

bool PCMSink::init_adf_elements_() {
  if ( this->sdk_audio_elements_.size() ){
    esph_log_e(TAG, "Called init, but elements already created.");
    return true;
  }
  assert( this->adf_raw_stream_reader_ == nullptr );

  raw_stream_cfg_t raw_cfg = {
      .type = AUDIO_STREAM_READER,
      .out_rb_size = 0,
  };
  this->adf_raw_stream_reader_ = raw_stream_init(&raw_cfg);
  audio_element_set_input_timeout(this->adf_raw_stream_reader_, 0);

  this->sdk_audio_elements_.push_back(this->adf_raw_stream_reader_);
  this->sdk_element_tags_.push_back("pcm_reader");
  return true;
}

void PCMSink::clear_adf_elements_(){
  //make sure that audio_element_deinit was called before for freeing allocated memory
  this->adf_raw_stream_reader_ = nullptr;
  this->sdk_audio_elements_.clear();
  this->sdk_element_tags_.clear();
}

int PCMSink::stream_read_bytes(char *buffer, int len) {
  int ret = audio_element_input(adf_raw_stream_reader_, buffer, len);
  if (ret < 0 && (ret != AEL_IO_TIMEOUT)) {
    audio_element_report_status(adf_raw_stream_reader_, AEL_STATUS_STATE_STOPPED);
  } else if (ret < 0) {
    return 0;
  }
  return ret;
}

void PCMSink::on_settings_request(AudioPipelineSettingsRequest &request) {
 if (request.bit_depth > 0 && (uint8_t) request.bit_depth != this->bits_per_sample_ ){
    if ( request.bit_depth == 16 || request.bit_depth == 24 || request.bit_depth == 32 )
    {
      esph_log_d(TAG, "Set bitdepth to %d", request.bit_depth );
      this->bits_per_sample_ = request.bit_depth;
    }
    else {
      request.failed = true;
      request.failed_by = this;
    }
  }

  if (request.final_sampling_rate == -1) {
    request.final_sampling_rate = 16000;
    request.final_bit_depth = this->bits_per_sample_;
    request.final_number_of_channels = 1;
  }
}


}  // namespace esp_adf
}  // namespace esphome

#endif
