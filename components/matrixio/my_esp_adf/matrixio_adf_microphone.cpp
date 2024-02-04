#include "matrixio_adf_microphone.h"
#include "../microphone_fir.h"

#include <driver/gpio.h>

#include <freertos/FreeRTOS.h>
#include <algorithm>
#include "raw_stream.h"

namespace esphome {
namespace matrixio {

#define CHUNK_SIZE 512

static const char *const TAG = "adf_matrixio_microphone";


static QueueHandle_t irq_queue;

static void irq_handler(void *args) {
  gpio_num_t gpio;
  gpio = static_cast<gpio_num_t>(esphome::matrixio::MICROPHONE_ARRAY_IRQ);
  xQueueSendToBackFromISR(irq_queue, &gpio, NULL);
}

static esp_err_t _matrixio_open(audio_element_handle_t self){
   xQueueReset(irq_queue);
   gpio_isr_handler_add(static_cast<gpio_num_t>(MICROPHONE_ARRAY_IRQ), irq_handler, NULL);
  //this_writer->flush_fpga_fifo_();
  //delay(10);
   esp_err_t res = audio_element_set_input_timeout(self, 2000 / portTICK_RATE_MS);
   return res;
}

static esp_err_t _matrixio_close(audio_element_handle_t self){
   gpio_isr_handler_remove(static_cast<gpio_num_t>(MICROPHONE_ARRAY_IRQ));
   return ESP_OK;
}

static audio_element_err_t _matrixio_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    //return (audio_element_err_t) len;
    MatrixIOMicrophone *matMicrophone = (MatrixIOMicrophone *) audio_element_getdata(self);
    size_t bytes_written = 0;
    
    gpio_num_t gpio;
    gpio = static_cast<gpio_num_t>(MICROPHONE_ARRAY_IRQ);

    //if (xQueueReceive(irq_queue, &gpio, (100 / portTICK_PERIOD_MS) ) )
    if (xQueueReceive(irq_queue, &gpio, ticks_to_wait))
    {
        memset( buffer, 0, len );
        uint8_t* pt = reinterpret_cast<unsigned char *>(buffer);
        matMicrophone->wb_read( pt, len );
        return (audio_element_err_t) len;
    }

    ESP_LOGW(TAG,"Buffer underrun.");
    return (audio_element_err_t) bytes_written;
}

static audio_element_err_t _adf_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
  //vTaskDelay( pdMS_TO_TICKS( 10 ) );
  //return (audio_element_err_t) in_len;
  int r_size = audio_element_input(self, in_buffer, in_len);
  int w_size = 0;
  if (r_size == AEL_IO_TIMEOUT) {
    memset(in_buffer, 0x00, in_len);
    r_size = in_len;
  }
  if (r_size > 0) {
    w_size = audio_element_output(self, in_buffer, r_size);
    audio_element_update_byte_pos(self, w_size);
  }
  return (audio_element_err_t) w_size;
}



void MatrixIOMicrophone::setup() {
  set_sampling_rate(16000);
  write_sampling_rate_and_gain_();
  write_fir_coeffs_();
  
  // IRQ handling
  irq_queue = xQueueCreate(10, sizeof(gpio_num_t));

  gpio_config_t gpioConfig;
  gpioConfig.pin_bit_mask = BIT(5); //GPIO_SEL_5;
  gpioConfig.mode = GPIO_MODE_INPUT;
  gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
  gpioConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
  gpioConfig.intr_type = GPIO_INTR_ANYEDGE;

  gpio_config(&gpioConfig);
  gpio_install_isr_service(0);
}

void MatrixIOMicrophone::dump_config(){
  esph_log_config(TAG, "Matrixio Microphones:");
}


void MatrixIOMicrophone::set_sampling_rate(uint32_t sampling_rate){
  for (int i = 0;; i++) {
    if (MIC_SAMPLING_FREQUENCIES[i][0] == 0){
      ESP_LOGE(TAG, "Unsupported sampling rate : %u", sampling_rate );
      mark_failed();
      return;
    }

    if (MIC_SAMPLING_FREQUENCIES[i][0] == sampling_rate) {
      sampling_rate_ = MIC_SAMPLING_FREQUENCIES[i][0];
      pdm_decimation_ = MIC_SAMPLING_FREQUENCIES[i][1];
      gain_ = MIC_SAMPLING_FREQUENCIES[i][2];
      break;
    }
  }
}

void MatrixIOMicrophone::write_fir_coeffs_(){
  const FIRCoeff* coeff_list = FIR_default;
  for (int i = 0;; i++) {
    if (coeff_list[i].rate_ == 0) {
      ESP_LOGE(TAG, "Unsupported sampling frequency for setting FIR coeffs: %u", this->sampling_rate_);
      mark_failed();
      return;
    }
    if (coeff_list[i].rate_ == this->sampling_rate_) {
      if (coeff_list[i].coeff_.size() == NUMBER_OF_FIR_TAPS) {
        std::valarray<int16_t> fir_coeff_ = coeff_list[i].coeff_;
        wb_write(reinterpret_cast<uint8_t *>(&fir_coeff_[0]), fir_coeff_.size() * sizeof(int16_t));
        return;
      } else {
        ESP_LOGE(TAG, "Size FIR Filter must be : %u", NUMBER_OF_FIR_TAPS );
        mark_failed();
      }
    }
  }
}

void MatrixIOMicrophone::write_sampling_rate_and_gain_(){
  conf_write(0x06, pdm_decimation_);
  conf_write(0x07, gain_);
}

void MatrixIOMicrophone::init_adf_elements_(){
  if( this->sdk_audio_elements_.size() > 0 )
    return;
  
  audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
  cfg.open = _matrixio_open;
  cfg.close = _matrixio_close;
  cfg.process = _adf_process;
  cfg.read = _matrixio_read;
  //cfg.destroy = nullptr;
  cfg.task_stack = DEFAULT_ELEMENT_STACK_SIZE; //-1; //3072+512;
  cfg.task_prio = 0;
  cfg.task_core = 0;
  cfg.stack_in_ext = false;
  cfg.out_rb_size = 4 * CHUNK_SIZE;
  cfg.multi_out_rb_num = 0;
  cfg.tag = "matrixio";
  cfg.buffer_len = CHUNK_SIZE;

  this->audio_raw_stream_ = audio_element_init(&cfg);
  audio_element_setdata(this->audio_raw_stream_, this);
  
  sdk_audio_elements_.push_back( this->audio_raw_stream_ );
  sdk_element_tags_.push_back("audio_in");   
}


}
}