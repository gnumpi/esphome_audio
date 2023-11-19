#include "speaker.h"
#include <freertos/FreeRTOS.h>

namespace esphome {
namespace matrixio {

static const size_t BUFFER_COUNT = 20;

void Speaker::setup(){
  ESP_LOGCONFIG(TAG, "Setting up MatrixIO Speaker...");
  this->set_pcm_sampling_frequency(16000);
  this->write_output_();
  this->unmute();
  this->write_volume_();
  this->buffer_queue_ = xQueueCreate(BUFFER_COUNT, sizeof(DataEvent));
  this->event_queue_ = xQueueCreate(BUFFER_COUNT, sizeof(TaskEvent));
}

void Speaker::dump_config(){
  esph_log_config(TAG, "Matrixio Speaker:");
  uint32_t sampling_frequency = this->read_pcm_sampling_frequency();
  uint8_t volume = this->get_volume();
  esph_log_config(TAG, "  Sampling frequency: %uHz", (unsigned) (sampling_frequency));
  if (this->output_ == kHeadPhone ){
    esph_log_config(TAG, "  Output: Headphone");
  }
  else if (this->output_ == kSpeaker ){
    esph_log_config(TAG, "  Output: Speaker");
  }
  else {
    esph_log_config(TAG, "  Output not set");

  }
  esph_log_config(TAG, "  Volume: %d%%", volume); 
}

void Speaker::start() { this->state_ = speaker::STATE_STARTING; }
void Speaker::start_() {
  this->state_ = speaker::STATE_RUNNING;
  xTaskCreate(Speaker::player_task, "speaker_task", 8192, (void *) this, 0, &this->player_task_handle_);
}

void Speaker::stop() {
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

void Speaker::watch_() {
  TaskEvent event;
  if (xQueueReceive(this->event_queue_, &event, 0) == pdTRUE) {
    switch (event.type) {
      case TaskEventType::STARTING:
        ESP_LOGD(TAG, "Starting MatrixIO Audio Speaker");
        break;
      case TaskEventType::STARTED:
        ESP_LOGD(TAG, "Started MatrixIO Audio Speaker");
        break;
      case TaskEventType::STOPPING:
        ESP_LOGD(TAG, "Stopping MatrixIO Audio Speaker");
        break;
      case TaskEventType::PLAYING:
        this->status_clear_warning();
        break;
      case TaskEventType::STOPPED:
        this->state_ = speaker::STATE_STOPPED;
        vTaskDelete(this->player_task_handle_);
        this->player_task_handle_ = nullptr;
        xQueueReset(this->buffer_queue_);
        ESP_LOGD(TAG, "Stopped MatrixIO Audio Speaker");
        break;
      case TaskEventType::WARNING:
        ESP_LOGW(TAG, "Error writing to I2S: %s", esp_err_to_name(event.err));
        this->status_set_warning();
        break;
    }
  }
}

void Speaker::loop() {
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

void Speaker::player_task(void *params) {
  Speaker *this_speaker = (Speaker *) params;

  TaskEvent event;
  event.type = TaskEventType::STARTING;
  xQueueSend(this_speaker->event_queue_, &event, portMAX_DELAY);

  this_speaker->flush_fpga_fifo_();
  delay(10);
  
  event.type = TaskEventType::STARTED;
  xQueueSend(this_speaker->event_queue_, &event, portMAX_DELAY);

  DataEvent data_event;
  

  while (true) {
    if (xQueueReceive(this_speaker->buffer_queue_, &data_event, 1000 / portTICK_PERIOD_MS) != pdTRUE) {
      break;  // End of audio from main thread
    }
    if (data_event.stop) {
      // Stop signal from main thread
      xQueueReset(this_speaker->buffer_queue_);  // Flush queue
      break;
    }
    float sample_time = 1.0 / this_speaker->pcm_sampling_frequency_;
    uint16_t fifo_status = this_speaker->get_fpga_fifo_status_();

    if (fifo_status > FPGA_FIFO_SIZE * 3 / 4) {
      int sleep = int(MAX_WRITE_LENGTH * sample_time * 1000);
      delay(sleep);
    }
    
    this_speaker->wb_write(data_event.data, data_event.len);
    event.type = TaskEventType::PLAYING;
    xQueueSend(this_speaker->event_queue_, &event, portMAX_DELAY);
  }

  event.type = TaskEventType::STOPPING;
  xQueueSend(this_speaker->event_queue_, &event, portMAX_DELAY);
  
  delay(10);

  event.type = TaskEventType::STOPPED;
  xQueueSend(this_speaker->event_queue_, &event, portMAX_DELAY);

  while (true) {
    delay(10);
  }
}

size_t Speaker::play(const uint8_t *data, size_t length) {
  if (this->state_ != speaker::STATE_RUNNING && this->state_ != speaker::STATE_STARTING) {
    this->start();
  }
  
   //hardcoded mono to stereo (send each sample twice)
  uint16_t* dst = buffer;
  const uint16_t* src = reinterpret_cast<const uint16_t*>(data);
  for (size_t c=0; c < length / sizeof(uint16_t); c++ ){
      memcpy( dst++, src,   sizeof(uint16_t) );
      memcpy( dst++, src++, sizeof(uint16_t) );
  }
  size_t remaining = length * 2;
  
  size_t index = 0;
  while (remaining > 0) {
    DataEvent event;
    event.stop = false;
    size_t to_send_length = std::min(remaining, BUFFER_SIZE);
    event.len = to_send_length;
    memcpy(event.data, reinterpret_cast<uint8_t*>(buffer) + index, to_send_length);
    if (xQueueSend(this->buffer_queue_, &event, 0) != pdTRUE) {
      return index / 2;
    }
    remaining -= to_send_length;
    index += to_send_length;
  }
  return index / 2; // caller doesn't know about duplication
}

uint16_t Speaker::get_fpga_fifo_status_() {
  uint16_t write_pointer;
  uint16_t read_pointer;
  this->reg_read(0x802, &read_pointer);
  this->reg_read(0x803, &write_pointer);

  if (write_pointer > read_pointer)
    return write_pointer - read_pointer;
  else
    return FPGA_FIFO_SIZE - read_pointer + write_pointer;
}

void Speaker::flush_fpga_fifo_() {
  this->conf_write(12, 0x0001);
  this->conf_write(12, 0x0000);
}

void Speaker::write_output_() {
  this->conf_write(11, this->output_);
}

void Speaker::set_output(OutputSelector output_selector) {
  this->output_ = output_selector;
}

void Speaker::mute(){
  this->conf_write(10, kMute );
  this->mute_status_ = kMute;
}

void Speaker::unmute(){
  this->conf_write(10, kUnMute );
  this->mute_status_ = kUnMute;
}

void Speaker::set_pcm_sampling_frequency(uint32_t sampling_frequency){
  uint16_t pcm_constant;
  for (int i = 0;; i++) {
    if (PCM_SAMPLING_FREQUENCIES[i][0] == 0){
        ESP_LOGE(TAG, "Unsupported sampling frequency: %d", sampling_frequency);
        this->mark_failed();  
    } 
    if ( sampling_frequency == PCM_SAMPLING_FREQUENCIES[i][0]) {
      this->pcm_sampling_frequency_ = PCM_SAMPLING_FREQUENCIES[i][0];
      pcm_constant = PCM_SAMPLING_FREQUENCIES[i][1];
      break;
    }
  }
  this->conf_write( 9, pcm_constant);
}

uint32_t Speaker::read_pcm_sampling_frequency(){
  uint16_t pcm_constant;
  this->conf_read(9, &pcm_constant);
  for (int i = 0;; i++) {
    if (pcm_constant == PCM_SAMPLING_FREQUENCIES[i][1]) {
      return PCM_SAMPLING_FREQUENCIES[i][0];
    }
  }
  ESP_LOGE(TAG, "Reading sampling frequency faild. Got : %d", pcm_constant);
  return 0;
}

void Speaker::set_volume(uint8_t volume_percentage){
  this->volume_ = volume_percentage > 100 ? 100 : volume_percentage; 
}

void Speaker::write_volume_(){
  uint16_t volume_constant = (100 - this->volume_) * MAX_VOLUME_VALUE / 100;
  this->conf_write(8, volume_constant);
}

uint8_t Speaker::get_volume(){
  uint16_t volume_constant;
  this->conf_read(8, &volume_constant);
  return (volume_constant * -100) / MAX_VOLUME_VALUE + 100;
}

} //namespace matrixio
} //namespace esphome