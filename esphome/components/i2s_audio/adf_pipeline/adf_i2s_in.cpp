#include "adf_i2s_in.h"
#ifdef USE_ESP_IDF

#include "i2s_stream_mod.h"
#include "../../adf_pipeline/adf_pipeline.h"
#include "../../adf_pipeline/sdk_ext.h"

#ifdef I2S_EXTERNAL_ADC
#include "../external_adc.h"
#endif
namespace esphome {
using namespace esp_adf;
namespace i2s_audio {

static const char *const TAG = "adf_i2s_in";

bool ADFElementI2SIn::init_adf_elements_() {
  if (this->sdk_audio_elements_.size() > 0)
    return true;

#ifdef I2S_EXTERNAL_ADC
  if (this->external_adc_ != nullptr){
    this->external_adc_->init_device();
  }
#endif
  if ( !this->claim_i2s_access() ){
    return false;
  }

  i2s_driver_config_t i2s_config = this->get_i2s_cfg();

  i2s_stream_cfg_t i2s_stream_cfg = {
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
      .uninstall_drv = false,
      .need_expand = false,
      .expand_src_bits = I2S_BITS_PER_SAMPLE_16BIT,
  };

  this->adf_i2s_stream_reader_ = i2s_stream_init(&i2s_stream_cfg);
  this->adf_i2s_stream_reader_->buf_size = 2 * 256;

  this->install_i2s_driver(i2s_config);

#ifdef I2S_EXTERNAL_ADC
  if (this->external_adc_ != nullptr){
    this->external_adc_->apply_i2s_settings(i2s_config);
  }
#endif

  audio_element_set_music_info(this->adf_i2s_stream_reader_, this->sample_rate_, 1, this->bits_per_sample_);

  sdk_audio_elements_.push_back(this->adf_i2s_stream_reader_);
  sdk_element_tags_.push_back("i2s_in");

  return true;
};

bool ADFElementI2SIn::is_ready(){
  if( !this->claim_i2s_access() )
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
  this->uninstall_i2s_driver();
}


}  // namespace i2s_audio
}  // namespace esphome

#endif
