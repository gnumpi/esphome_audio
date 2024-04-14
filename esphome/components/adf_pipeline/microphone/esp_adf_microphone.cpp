#include "esp_adf_microphone.h"

#ifdef USE_ESP_IDF

#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esp_log.h"

namespace esphome {
namespace esp_adf {

static const char *const TAG = "esp_adf_pipeline.microphone";

void ADFMicrophone::setup() {
  esp_log_level_set("wifi", ESP_LOG_WARN);
}

void ADFMicrophone::dump_config() {
  esph_log_config(TAG, "ADF-Microphone");
  ADFPipelineController::dump_config();
}

void ADFMicrophone::start() {
  if ( this->state_ == microphone::STATE_RUNNING){
    return;
  }
  pipeline.start();
  this->state_ = microphone::STATE_STARTING;
}

void ADFMicrophone::stop() {
  this->state_ = microphone::STATE_STOPPING;
  pipeline.stop();
}

// len and return size are both in bytes
size_t ADFMicrophone::read(int16_t *buf, size_t len) {
    len = (len << 1) >> 1;
    if (this->pcm_stream_.get_bits_per_sample() == 32 || this->pcm_stream_.get_bits_per_sample() == 24)
    {
      len = ( len << 2 ) >> 2;
      size_t bytes_read = this->pcm_stream_.stream_read_bytes((char *) buf, len << 1  );
      size_t samples_read = bytes_read / sizeof(int32_t);
      if( samples_read == 0 ){
        return 0;
      }

      std::vector<int16_t> samples;
      uint8_t shift = 16 - this->gain_log2_ ;
      samples.resize(samples_read);
      for (size_t i = 0; i < samples_read; i++) {
        int32_t temp = ( reinterpret_cast<int32_t *>(buf)[i] ) >> shift;
         samples[i] = (int16_t) clamp<int32_t>(temp, INT16_MIN, INT16_MAX );
      }
      memcpy(buf, samples.data(), samples_read * sizeof(int16_t));
      return samples_read * sizeof(int16_t);
    }
    size_t bytes_read = 0;
    bytes_read =  this->pcm_stream_.stream_read_bytes((char *) buf, len);
    return bytes_read;
}

void ADFMicrophone::on_pipeline_state_change(PipelineState state) {
  switch (state) {
    case PipelineState::STARTING:
      this->state_ = microphone::STATE_STARTING;
      break;
    case PipelineState::RUNNING:
      this->state_ = microphone::STATE_RUNNING;
      break;
    case PipelineState::STOPPING:
      this->state_ = microphone::STATE_STOPPING;
      break;
    case PipelineState::UNINITIALIZED:
      this->state_ = microphone::STATE_STOPPED;
      break;
    case PipelineState::PAUSED:
      ESP_LOGI(TAG, "pipeline paused");
      this->state_ = microphone::STATE_STOPPED;
      break;
    case PipelineState::STOPPED:
      this->state_ = microphone::STATE_STOPPED;
      break;
    case PipelineState::PAUSING:
    case PipelineState::RESUMING:
    case PipelineState::PREPARING:
    case PipelineState::DESTROYING:
      break;
    }
}

}  // namespace esp_adf
}  // namespace esphome
#endif
