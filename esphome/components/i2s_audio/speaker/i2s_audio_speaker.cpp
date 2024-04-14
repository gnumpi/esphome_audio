#include "i2s_audio_speaker.h"

#ifdef USE_ESP32

#include <driver/i2s.h>
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#ifdef I2S_EXTERNAL_DAC
#include "../external_dac.h"
#endif
namespace esphome {
namespace i2s_audio {

static const size_t BUFFER_COUNT = 20;

static const char *const TAG = "i2s_audio.speaker";

void I2SAudioSpeaker::setup() {
  ESP_LOGCONFIG(TAG, "Setting up I2S Audio Speaker...");

  this->set_bits_per_sample(I2S_BITS_PER_SAMPLE_16BIT);
  this->set_channel(I2S_CHANNEL_FMT_ONLY_RIGHT);
  this->set_sample_rate(16000);

  this->buffer_queue_ = xQueueCreate(BUFFER_COUNT, sizeof(DataEvent));
  this->event_queue_ = xQueueCreate(BUFFER_COUNT, sizeof(TaskEvent));
}

void I2SAudioSpeaker::dump_config() {
  this->dump_i2s_settings();
}


void I2SAudioSpeaker::start() { this->state_ = speaker::STATE_STARTING; }

void I2SAudioSpeaker::start_() {
  if (!this->claim_i2s_access()) {
    return;  // Waiting for another i2s component to return lock
  }
  this->state_ = speaker::STATE_RUNNING;

  xTaskCreate(I2SAudioSpeaker::player_task, "speaker_task", 8192, (void *) this, 1, &this->player_task_handle_);
}

void I2SAudioSpeaker::player_task(void *params) {
  I2SAudioSpeaker *this_speaker = (I2SAudioSpeaker *) params;

#ifdef I2S_EXTERNAL_DAC
  if( this_speaker->external_dac_ != nullptr ){
    this_speaker->external_dac_->init_device();
  }
#endif

  TaskEvent event;
  event.type = TaskEventType::STARTING;
  xQueueSend(this_speaker->event_queue_, &event, portMAX_DELAY);

  i2s_driver_config_t config = this_speaker->get_i2s_cfg();
  config.mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX);

#if SOC_I2S_SUPPORTS_DAC
  if (this_speaker->internal_dac_mode_ != I2S_DAC_CHANNEL_DISABLE) {
    config.mode = (i2s_mode_t) (config.mode | I2S_MODE_DAC_BUILT_IN);
  }
#endif

  bool success = this_speaker->install_i2s_driver(config);
  if (!success) {
    event.type = TaskEventType::WARNING;
    event.err = -2;
    xQueueSend(this_speaker->event_queue_, &event, 0);
    event.type = TaskEventType::STOPPED;
    xQueueSend(this_speaker->event_queue_, &event, 0);
    while (true) {
      delay(10);
    }
  }

#if SOC_I2S_SUPPORTS_DAC
  if (this_speaker->internal_dac_mode_ == I2S_DAC_CHANNEL_DISABLE) {
#endif
    i2s_pin_config_t pin_config = this_speaker->parent_->get_pin_config();
    pin_config.data_out_num = this_speaker->dout_pin_;

    i2s_set_pin(this_speaker->parent_->get_port(), &pin_config);
#if SOC_I2S_SUPPORTS_DAC
  } else {
    i2s_set_dac_mode(this_speaker->internal_dac_mode_);
  }
#endif

#ifdef I2S_EXTERNAL_DAC
  if( this_speaker->external_dac_ != nullptr ){
    this_speaker->external_dac_->apply_i2s_settings(config);
  }
#endif

  DataEvent data_event;

  event.type = TaskEventType::STARTED;
  xQueueSend(this_speaker->event_queue_, &event, portMAX_DELAY);

  while (true) {
    if (xQueueReceive(this_speaker->buffer_queue_, &data_event, 100 / portTICK_PERIOD_MS) != pdTRUE) {
      break;  // End of audio from main thread
    }
    if (data_event.stop) {
      // Stop signal from main thread
      xQueueReset(this_speaker->buffer_queue_);  // Flush queue
      break;
    }
    size_t bytes_written;
    esp_err_t err = i2s_write(this_speaker->parent_->get_port(), data_event.data, data_event.len, &bytes_written,
                                (10 / portTICK_PERIOD_MS));
    if (err != ESP_OK) {
      event = {.type = TaskEventType::WARNING, .err = err};
      xQueueSend(this_speaker->event_queue_, &event, portMAX_DELAY);
      continue;
    }

    event.type = TaskEventType::PLAYING;
    xQueueSend(this_speaker->event_queue_, &event, portMAX_DELAY);
  }

  i2s_zero_dma_buffer(this_speaker->parent_->get_port());

  event.type = TaskEventType::STOPPING;
  xQueueSend(this_speaker->event_queue_, &event, portMAX_DELAY);

  this_speaker->uninstall_i2s_driver();

  event.type = TaskEventType::STOPPED;
  xQueueSend(this_speaker->event_queue_, &event, portMAX_DELAY);

  while (true) {
    delay(10);
  }
}

void I2SAudioSpeaker::stop() {
  if (this->state_ == speaker::STATE_STOPPED)
    return;
  if (this->state_ == speaker::STATE_STARTING) {
    this->state_ = speaker::STATE_STOPPED;
    return;
  }
  this->state_ = speaker::STATE_STOPPING;
  DataEvent data;
  data.stop = true;
  xQueueSendToFront(this->buffer_queue_, &data, portMAX_DELAY);
}

void I2SAudioSpeaker::watch_() {
  TaskEvent event;
  if (xQueueReceive(this->event_queue_, &event, 0) == pdTRUE) {
    switch (event.type) {
      case TaskEventType::STARTING:
        ESP_LOGD(TAG, "Starting I2S Audio Speaker");
        break;
      case TaskEventType::STARTED:
        ESP_LOGD(TAG, "Started I2S Audio Speaker");
        break;
      case TaskEventType::STOPPING:
        ESP_LOGD(TAG, "Stopping I2S Audio Speaker");
        break;
      case TaskEventType::PLAYING:
        this->status_clear_warning();
        break;
      case TaskEventType::STOPPED:
        this->state_ = speaker::STATE_STOPPED;
        vTaskDelete(this->player_task_handle_);
        this->player_task_handle_ = nullptr;
        this->release_i2s_access();
        xQueueReset(this->buffer_queue_);
        ESP_LOGD(TAG, "Stopped I2S Audio Speaker");
        break;
      case TaskEventType::WARNING:
        ESP_LOGW(TAG, "Error writing to I2S: %s", esp_err_to_name(event.err));
        this->status_set_warning();
        break;
    }
  }
}

void I2SAudioSpeaker::loop() {
  switch (this->state_) {
    case speaker::STATE_STARTING:
      this->start_();
      break;
    case speaker::STATE_RUNNING:
    case speaker::STATE_STOPPING:
      this->watch_();
      break;
    case speaker::STATE_STOPPED:
      break;
  }
}

size_t I2SAudioSpeaker::play(const uint8_t *data, size_t length) {
  if (this->state_ != speaker::STATE_RUNNING && this->state_ != speaker::STATE_STARTING) {
    this->start();
  }
  size_t remaining = length;
  size_t index = 0;
  while (remaining > 0) {
    DataEvent event;
    event.stop = false;
    size_t to_send_length = std::min(remaining, BUFFER_SIZE);
    event.len = to_send_length;
    memcpy(event.data, data + index, to_send_length);
    if (xQueueSend(this->buffer_queue_, &event, 0) != pdTRUE) {
      return index;
    }
    remaining -= to_send_length;
    index += to_send_length;
  }
  return index;
}

bool I2SAudioSpeaker::has_buffered_data() const { return uxQueueMessagesWaiting(this->buffer_queue_) > 0; }

}  // namespace i2s_audio
}  // namespace esphome

#endif  // USE_ESP32
