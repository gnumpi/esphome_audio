#include "adf_i2s_out.h"
#ifdef USE_ESP_IDF

#include "../../adf_pipeline/sdk_ext.h"
#include "i2s_stream_mod.h"
#include "../../adf_pipeline/adf_pipeline.h"

#ifdef I2S_EXTERNAL_DAC
#include "../external_dac.h"
#endif
namespace esphome {
using namespace esp_adf;
namespace i2s_audio {

static const char *const TAG = "adf_i2s_out";


void ADFElementI2SOut::setup() {
}

bool ADFElementI2SOut::init_adf_elements_() {
  if (this->sdk_audio_elements_.size() > 0)
    return true;

 if ( !this->claim_i2s_access() ){
    return false;
  }

#ifdef I2S_EXTERNAL_DAC
  if (this->external_dac_ != nullptr){
    this->external_dac_->init_device();
  }
#endif

  i2s_driver_config_t i2s_config = this->get_i2s_cfg();

  i2s_stream_cfg_t i2s_cfg = {
      .type = AUDIO_STREAM_WRITER,
      .i2s_config = i2s_config,
      .i2s_port = this->parent_->get_port(),
      .use_alc = this->use_adf_alc_,
      .volume = 0,
      .out_rb_size = (4 * 1024),
      .task_stack = I2S_STREAM_TASK_STACK,
      .task_core = I2S_STREAM_TASK_CORE,
      .task_prio = I2S_STREAM_TASK_PRIO,
      .stack_in_ext = false,
      .multi_out_num = 0,
      .uninstall_drv = false,
      .need_expand = false,
      .expand_src_bits = I2S_BITS_PER_SAMPLE_16BIT,
  };

  this->adf_i2s_stream_writer_ = i2s_stream_init(&i2s_cfg);
  this->adf_i2s_stream_writer_->buf_size = 1 * 1024;

  this->install_i2s_driver(i2s_config);

#ifdef I2S_EXTERNAL_DAC
  if (this->external_dac_ != nullptr){
    this->external_dac_->apply_i2s_settings(i2s_config);
  }
#endif

  sdk_audio_elements_.push_back(this->adf_i2s_stream_writer_);
  sdk_element_tags_.push_back("i2s_out");

  return true;
}

void ADFElementI2SOut::clear_adf_elements_(){
  this->sdk_audio_elements_.clear();
  this->sdk_element_tags_.clear();
  this->uninstall_i2s_driver();
}

bool ADFElementI2SOut::is_ready(){
  return this->claim_i2s_access();
}

void ADFElementI2SOut::on_settings_request(AudioPipelineSettingsRequest &request) {
  if ( !this->adf_i2s_stream_writer_ ){
    return;
  }

  if (this->is_adjustable()){
    bool rate_bits_channels_updated = false;
    if (request.sampling_rate > 0 && (uint32_t) request.sampling_rate != this->sample_rate_) {
      this->sample_rate_ = request.sampling_rate;
      rate_bits_channels_updated = true;
    }

    if(request.number_of_channels > 0 && (uint8_t) request.number_of_channels != this->num_of_channels())
    {
      this->channel_fmt_ = request.number_of_channels == 1 ? I2S_CHANNEL_FMT_ONLY_RIGHT : I2S_CHANNEL_FMT_RIGHT_LEFT;
      rate_bits_channels_updated = true;
    }

    if (request.bit_depth > 0 && (uint8_t) request.bit_depth != this->bits_per_sample_) {
      bool supported = request.bit_depth == 16;
      if (!supported) {
        request.failed = true;
        request.failed_by = this;
        return;
      }
      this->bits_per_sample_ = (i2s_bits_per_sample_t) request.bit_depth;
      rate_bits_channels_updated = true;
    }

    if (rate_bits_channels_updated) {

      audio_element_set_music_info(this->adf_i2s_stream_writer_,this->sample_rate_, this->num_of_channels(), this->bits_per_sample_ );

      esph_log_d(TAG, "update i2s clk settings: rate:%d bits:%d ch:%d",this->sample_rate_, this->bits_per_sample_, this->num_of_channels());
      if (i2s_stream_set_clk(this->adf_i2s_stream_writer_, this->sample_rate_, this->bits_per_sample_,
                            this->num_of_channels()) != ESP_OK) {
        esph_log_e(TAG, "error while setting sample rate and bit depth,");
        request.failed = true;
        request.failed_by = this;
        return;
      }
    }
  }

  // final pipeline settings are unset
  if (request.final_sampling_rate == -1) {
    esph_log_d(TAG, "Set final i2s settings: %d", this->sample_rate_);
    request.final_sampling_rate = this->sample_rate_;
    request.final_bit_depth = this->bits_per_sample_;
    request.final_number_of_channels = this->num_of_channels();
  } else if (
       request.final_sampling_rate != this->sample_rate_
    || request.final_bit_depth != this->bits_per_sample_
    || request.final_number_of_channels != this->num_of_channels()
  )
  {
    request.failed = true;
    request.failed_by = this;
  }

  if (this->use_adf_alc_ && request.target_volume > -1) {
    int target_volume = (int) (request.target_volume * 64.) - 32;
    if (i2s_alc_volume_set(this->adf_i2s_stream_writer_, target_volume) != ESP_OK) {
      esph_log_e(TAG, "error setting volume to %d", target_volume);
      request.failed = true;
      request.failed_by = this;
      return;
    }
  }
#ifdef I2S_EXTERNAL_DAC
  if( this->external_dac_ != nullptr && request.target_volume > -1){
    this->external_dac_->set_volume( request.target_volume);
  }
#endif
}

}  // namespace i2s_audio
}  // namespace esphome
#endif
