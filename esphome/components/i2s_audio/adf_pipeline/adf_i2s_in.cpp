#include "adf_i2s_in.h"
#ifdef USE_ESP_IDF

#include <i2s_stream.h>

namespace esphome {
using namespace esp_adf;
namespace i2s_audio {

void ADFElementI2SIn::init_adf_elements_(){
    if( this->sdk_audio_elements_.size() > 0 )
    return;

    i2s_driver_config_t i2s_config = {
      .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = 16000,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,
      .dma_buf_count = 8,
      .dma_buf_len = 128,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0,
      .mclk_multiple = I2S_MCLK_MULTIPLE_256,
      .bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT,
  };

  i2s_stream_cfg_t i2s_cfg = {
      .type = AUDIO_STREAM_READER,
      .i2s_config = i2s_config,
      .i2s_port = this->parent_->get_port(),
      .use_alc = false,
      .volume = 0,
      .out_rb_size = I2S_STREAM_RINGBUFFER_SIZE,
      .task_stack = I2S_STREAM_TASK_STACK,
      .task_core = I2S_STREAM_TASK_CORE,
      .task_prio = I2S_STREAM_TASK_PRIO,
      .stack_in_ext = false,
      .multi_out_num = 0,
      .uninstall_drv = true,
      .need_expand = false,
      .expand_src_bits = I2S_BITS_PER_SAMPLE_16BIT,
  };

  this->adf_i2s_stream_reader_ = i2s_stream_init(&i2s_cfg);

  i2s_pin_config_t pin_config = this->parent_->get_pin_config();    
  pin_config.data_in_num = this->din_pin_;
  i2s_set_pin(this->parent_->get_port(), &pin_config);
        
  sdk_audio_elements_.push_back( this->adf_i2s_stream_reader_ );
  sdk_element_tags_.push_back("i2s_in");   
};


}
}

#endif