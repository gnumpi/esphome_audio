#include "esp_adf_speaker.h"

#ifdef USE_ESP_IDF

#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace esp_adf {

static const char *const TAG = "esp_adf.speaker";

void ADFSpeaker::setup() {
  ESP_LOGCONFIG(TAG, "Setting up ESP ADF Speaker...");
}

void ADFSpeaker::dump_config() {
  ESP_LOGCONFIG(TAG, "ESP ADF Speaker Configs:");
}


void ADFSpeaker::start() { this->state_ = speaker::STATE_STARTING; }

void ADFSpeaker::start_() {
   pipeline.start();
}

void ADFSpeaker::stop() {
  if (this->state_ == speaker::STATE_STOPPED)
    return;
  this->state_ = speaker::STATE_STOPPING;
  pipeline.stop();
}

void ADFSpeaker::on_pipeline_state_change(PipelineState state) {
   switch (state) {
      case PipelineState::PREPARING:
        this->request_pipeline_settings_();
        break;
      case PipelineState::STARTING:
        break;
      case PipelineState::STOPPING:
        this->state_ = speaker::STATE_STOPPING;
        break;
      case PipelineState::RUNNING:
        this->state_ = speaker::STATE_RUNNING;
        break;
      case PipelineState::UNINITIALIZED:
      case PipelineState::STOPPED:
        this->state_ = speaker::STATE_STOPPED;
        break;
      case PipelineState::PAUSED:
        ESP_LOGI(TAG, "pipeline paused");
        break;
      case PipelineState::PAUSING:
      case PipelineState::RESUMING:
      case PipelineState::DESTROYING:
        break;
   }
}

void ADFSpeaker::loop() {
  this->pipeline.loop();
  switch (this->state_) {
    case speaker::STATE_STARTING:
      this->start_();
      break;
    case speaker::STATE_RUNNING:
    case speaker::STATE_STOPPING:
    case speaker::STATE_STOPPED:
      break;
  }
}

size_t ADFSpeaker::play(const uint8_t *data, size_t length) {
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Failed to play audio, speaker is in failed state.");
    return 0;
  }
  if (this->state_ != speaker::STATE_RUNNING && this->state_ != speaker::STATE_STARTING) {
    this->start();
  }
  size_t remaining = length;
  size_t index = 0;
  while (remaining > 0) {
    size_t to_send_length = std::min(remaining, BUFFER_SIZE);
    pcm_stream_.stream_write( (char *) data + index, to_send_length );
    remaining -= to_send_length;
    index += to_send_length;
  }
  return index;
}

bool ADFSpeaker::has_buffered_data() const {
  return pcm_stream_.has_buffered_data();
}

void ADFSpeaker::request_pipeline_settings_(){
    AudioPipelineSettingsRequest request{&this->pcm_stream_};
    request.sampling_rate = 16000;
    request.bit_depth = 16;
    request.number_of_channels = 1;
    if (!this->pipeline.request_settings(request)) {
      esph_log_e(TAG, "Requested audio settings, didn't get accepted");
      this->pipeline.on_settings_request_failed(request);
    }
}


}  // namespace esp_adf
}  // namespace esphome

#endif  // USE_ESP_IDF
