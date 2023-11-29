#include "microphone_array.h"
#include "microphone_fir.h"

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

static const char *const TAG = "matrixio_microphone";

static QueueHandle_t irq_queue;

static void irq_handler(void *args) {
  gpio_num_t gpio;
  gpio = static_cast<gpio_num_t>(esphome::matrixio::MICROPHONE_ARRAY_IRQ);
  xQueueSendToBackFromISR(irq_queue, &gpio, NULL);
}

namespace esphome {
namespace matrixio {

void Microphone::setup() {
  this->set_sampling_rate(16000);
  this->write_config();

  // IRQ handling
  irq_queue = xQueueCreate(10, sizeof(gpio_num_t));

  gpio_config_t gpioConfig;
  gpioConfig.pin_bit_mask = GPIO_SEL_5;
  gpioConfig.mode = GPIO_MODE_INPUT;
  gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
  gpioConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
  gpioConfig.intr_type = GPIO_INTR_ANYEDGE;

  gpio_config(&gpioConfig);
  gpio_install_isr_service(0);
}

void Microphone::dump_config(){
  esph_log_config(TAG, "Matrixio Microphones:");
}

void Microphone::write_config(){
  this->write_sampling_rate_and_gain();
  this->write_fir_coeffs();
}

void Microphone::start(){
    ESP_LOGD(TAG, "Starting Microphone.");
    if (this->is_failed())
    {
       ESP_LOGE(TAG, "Microphone not ready.");
       return;
    }
    if (this->state_ == microphone::STATE_RUNNING)
        return;  // Already running
    this->state_ = microphone::STATE_STARTING;
    xQueueReset(irq_queue);
    gpio_isr_handler_add(static_cast<gpio_num_t>(MICROPHONE_ARRAY_IRQ), irq_handler, NULL);
  };

void Microphone::stop() {
  if (this->state_ == microphone::STATE_STOPPED || this->is_failed()) return;
  if (this->state_ == microphone::STATE_STARTING) {
      this->state_ = microphone::STATE_STOPPED;
      return;
  }

  gpio_isr_handler_remove(static_cast<gpio_num_t>(MICROPHONE_ARRAY_IRQ));
  this->state_ = microphone::STATE_STOPPING;
};

size_t Microphone::read(int16_t *buf, size_t len){
  gpio_num_t gpio;
  gpio = static_cast<gpio_num_t>(MICROPHONE_ARRAY_IRQ);

  if (xQueueReceive(irq_queue, &gpio, (100 / portTICK_PERIOD_MS) ) )
  {
    memset( buf, 0, len );
    uint8_t* pt = reinterpret_cast<unsigned char *>(buf);
    this->wb_read( pt, len );
    return len;
  }

  ESP_LOGW(TAG,"Buffer underrun.");
  return 0;
}

void Microphone::set_sampling_rate(uint32_t sampling_rate){
  for (int i = 0;; i++) {
    if (MIC_SAMPLING_FREQUENCIES[i][0] == 0){
      ESP_LOGE(TAG, "Unsupported sampling rate : %d", sampling_rate );
      this->mark_failed();
      return;
    }

    if (MIC_SAMPLING_FREQUENCIES[i][0] == sampling_rate) {
      this->sampling_rate_ = MIC_SAMPLING_FREQUENCIES[i][0];
      this->pdm_decimation_ = MIC_SAMPLING_FREQUENCIES[i][1];
      this->gain_ = MIC_SAMPLING_FREQUENCIES[i][2];
      break;
    }
  }
}

void Microphone::write_fir_coeffs(){
  const FIRCoeff* coeff_list = FIR_default;
  for (int i = 0;; i++) {
    if (coeff_list[i].rate_ == 0) {
      ESP_LOGE(TAG, "Unsupported sampling frequency for setting FIR coeffs: %d", this->sampling_rate_);
      this->mark_failed();
      return;
    }
    if (coeff_list[i].rate_ == this->sampling_rate_) {
      if (coeff_list[i].coeff_.size() == NUMBER_OF_FIR_TAPS) {
        std::valarray<int16_t> fir_coeff_ = coeff_list[i].coeff_;
        this->wb_write(reinterpret_cast<uint8_t *>(&fir_coeff_[0]), fir_coeff_.size() * sizeof(int16_t));
        return;
      } else {
        ESP_LOGE(TAG, "Size FIR Filter must be : %d", NUMBER_OF_FIR_TAPS );
        this->mark_failed();
      }
    }
  }
}

void Microphone::write_sampling_rate_and_gain(){
  this->conf_write(0x06, this->pdm_decimation_);
  this->conf_write(0x07, this->gain_);
}

void Microphone::read_() {
  const size_t samples_to_read = 512;
  std::vector<int16_t> samples;
  samples.resize(samples_to_read);
  size_t bytes_read = this->read(samples.data(), samples_to_read /  sizeof(int16_t) );
  samples.resize( bytes_read / sizeof(int16_t));
  this->data_callbacks_.call(samples);
}

void Microphone::start_() {
  this->state_ = microphone::STATE_RUNNING;
  this->high_freq_.start();
}

void Microphone::stop_() {
  this->state_ = microphone::STATE_STOPPED;
  this->high_freq_.stop();
}

void Microphone::loop() {
  switch (this->state_) {
    case microphone::STATE_STOPPED:
      break;
    case microphone::STATE_STARTING:
      this->start_();
      break;
    case microphone::STATE_RUNNING:
      if (this->data_callbacks_.size() > 0) {
        this->read_();
      }
      break;
    case microphone::STATE_STOPPING:
      this->stop_();
      break;
  }
}

} //namespace matrixio
} //namespace esphome
