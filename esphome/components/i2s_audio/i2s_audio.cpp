#include "i2s_audio.h"

#ifdef USE_ESP32

#include "esphome/core/log.h"

namespace esphome {
namespace i2s_audio {

static const char *const TAG = "i2s_audio";

void I2SAudioComponent::setup() {
  static i2s_port_t next_port_num = I2S_NUM_0;

  if (next_port_num >= I2S_NUM_MAX) {
    ESP_LOGE(TAG, "Too many I2S Audio components!");
    this->mark_failed();
    return;
  }

  this->port_ = next_port_num;
  next_port_num = (i2s_port_t) (next_port_num + 1);

  ESP_LOGCONFIG(TAG, "Setting up I2S Audio...");
}

bool I2SAudioComponent::set_mode_(int mode){
  bool success = false;
  this->lock();
  esph_log_d(TAG, "Set I2S Lock Mode: %d", mode);
  I2SAccessState req_state = mode == 1 ? I2SAccessState::EXCLUSIVE_RX : I2SAccessState::EXCLUSIVE_TX;
  if( this->access_state_ == I2SAccessState::FREE ){
    this->access_state_ = req_state;
  }
  success = this->access_state_ == req_state;
  this->unlock();
  return success;
  }

bool I2SAudioComponent::release_mode_(int mode){
  bool success = false;
  this->lock();
  esph_log_d(TAG, "Release I2S Lock Mode: %d", mode);
  I2SAccessState req_state = mode == 1 ? I2SAccessState::EXCLUSIVE_RX : I2SAccessState::EXCLUSIVE_TX;
  if( this->access_state_ == req_state ) {
    this->access_state_ = I2SAccessState::FREE;
  }
  success = this->access_state_ == I2SAccessState::FREE;
  this->unlock();
  return success;
}

i2s_driver_config_t I2SAudioComponent::get_i2s_cfg() const {
  uint8_t mode = I2S_MODE_MASTER;
  switch( this->access_mode_){
    case I2SAccessMode::EXCLUSIVE_RX:
      mode = I2S_MODE_MASTER | I2S_MODE_RX;
      break;
    case I2SAccessMode::EXCLUSIVE_TX:
      mode = I2S_MODE_MASTER | I2S_MODE_TX;
      break;
    case I2SAccessMode::DUPLEX:
      mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX;
      break;
  }
  i2s_driver_config_t config = {
      .mode = (i2s_mode_t) mode,
      .sample_rate = this->sample_rate_,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 128,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = I2S_PIN_NO_CHANGE,
      .mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT,
      .bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT,
#if SOC_I2S_SUPPORTS_TDM
      .chan_mask = I2S_CHANNEL_MONO,
      .total_chan = 0,
      .left_align = false,
      .big_edin = false,
      .bit_order_msb = false,
      .skip_msk = false,
#endif

  };
  return config;
}

bool I2SAudioComponent::install_i2s_driver(){
  bool success = false;
  this->lock();
  if( this->access_state_ == I2SAccessState::FREE ){
    i2s_driver_config_t i2s_cfg = this->get_i2s_cfg();
    success = ESP_OK == i2s_driver_install(this->get_port(), &i2s_cfg, 0, nullptr);
    i2s_pin_config_t pin_config = this->get_pin_config();
    if( this->audio_in_ != nullptr )
    {
      pin_config.data_in_num = this->audio_in_->get_din_pin();
    }
    if( this->audio_out_ != nullptr )
    {
      pin_config.data_out_num = this->audio_out_->get_dout_pin();
    }
    success &= ESP_OK == i2s_set_pin(this->get_port(), &pin_config);
  }
  this->unlock();
  return success;
}

bool I2SAudioComponent::uninstall_i2s_driver(){
  bool success = false;
  this->lock();
  i2s_zero_dma_buffer(this->get_port());
  esp_err_t err = i2s_driver_uninstall(this->get_port());
  if (err == ESP_OK) {
    success = true;
  }
  this->unlock();
  return success;
}



}  // namespace i2s_audio
}  // namespace esphome

#endif  // USE_ESP32
