#include "adf_i2s_in.h"
#ifdef USE_ESP_IDF

#include "i2s_stream_mod.h"
#include "../../adf_pipeline/adf_pipeline.h"
#include "../../adf_pipeline/sdk_ext.h"
#include "../external_adc.h"

namespace esphome {
using namespace esp_adf;
namespace i2s_audio {

static const char *const TAG = "adf_i2s_in";

bool ADFElementI2SIn::init_adf_elements_() {
  if (!this->parent_->set_read_mode() )
  {
    return false;
  }
  if (this->sdk_audio_elements_.size() > 0)
    return true;

  if (this->external_adc_ != nullptr){
    this->external_adc_->init_device();
  }

  i2s_bits_per_chan_t channel_bits  = I2S_BITS_PER_CHAN_DEFAULT;

  if ( this->bits_per_sample_ == I2S_BITS_PER_SAMPLE_24BIT)
  {
    channel_bits = I2S_BITS_PER_CHAN_32BIT;

  }

  i2s_driver_config_t i2s_config = {
      .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = this->sample_rate_,
      .bits_per_sample = this->bits_per_sample_,
      .channel_format = this->channel_,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      //.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = 256,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0,
      .mclk_multiple = I2S_MCLK_MULTIPLE_256,
      .bits_per_chan = channel_bits,
  #if SOC_I2S_SUPPORTS_TDM
      .chan_mask = I2S_CHANNEL_MONO,
      .total_chan = 0,
      .left_align = false,
      .big_edin = false,
      .bit_order_msb = false,
      .skip_msk = false,
#endif
  };

  if( this->pdm_){
      i2s_config.mode = (i2s_mode_t) (i2s_config.mode | I2S_MODE_PDM);
  }

    i2s_stream_cfg_t i2s_cfg = {
      .type = AUDIO_STREAM_READER,
      .i2s_config = i2s_config,
      .i2s_port = this->parent_->get_port(),
      .use_alc = false,
      .volume = 0,
      .out_rb_size = (8 * 256),
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
  this->adf_i2s_stream_reader_->buf_size = 2 * 256;

  i2s_pin_config_t pin_config = this->parent_->get_pin_config();
  pin_config.data_in_num = this->din_pin_;
  i2s_set_pin(this->parent_->get_port(), &pin_config);

  if (this->external_adc_ != nullptr){
    this->external_adc_->apply_i2s_settings(i2s_config);
  }

  audio_element_set_music_info(this->adf_i2s_stream_reader_, this->sample_rate_, 1, this->bits_per_sample_);

  uint32_t bits_cfg = I2S_BITS_PER_SAMPLE_16BIT;
  if ( this->bits_per_sample_ == I2S_BITS_PER_SAMPLE_24BIT)
  {
    bits_cfg = (I2S_BITS_PER_CHAN_32BIT << 16) | I2S_BITS_PER_SAMPLE_24BIT;
  }
  else if (( this->bits_per_sample_ == I2S_BITS_PER_SAMPLE_32BIT) )
  {
    bits_cfg = I2S_BITS_PER_SAMPLE_32BIT;
  }

  sdk_audio_elements_.push_back(this->adf_i2s_stream_reader_);
  sdk_element_tags_.push_back("i2s_in");

  return true;
};

bool ADFElementI2SIn::is_ready(){
  if( !this->parent_->set_read_mode() )
  {
    return false;
  }
  if( !this->valid_settings_){
    AudioPipelineSettingsRequest request{this};
    request.sampling_rate = this->sample_rate_;
    request.bit_depth = this->bits_per_sample_;
    request.number_of_channels = 2;
    this->valid_settings_ = pipeline_->request_settings(request);
  }
  return this->valid_settings_;
}

void ADFElementI2SIn::clear_adf_elements_(){
  this->sdk_audio_elements_.clear();
  this->sdk_element_tags_.clear();
  this->parent_->release_read_mode();
}


}  // namespace i2s_audio
}  // namespace esphome

#endif
