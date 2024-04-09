#include "external_dac.h"

#ifdef I2S_EXTERNAL_DAC

#include "esphome/core/log.h"

namespace esphome {
namespace i2s_audio {

static const char *const TAG = "i2s_external_dac";

bool AW88298::init_device(){
  if( this->enable_pin_ != nullptr ){
    this->enable_pin_->digital_write(true);
  }

  uint16_t val = 0x0673;
  this->write_bytes_16(0x61, &val, 1 ); // AW88298_REG_BSTCTRL2

  // enabled I2EN and switch to operation mode
  val = 0x4042; // I2SEN=1 AMPPD=1 PWDN=0
  this->write_bytes_16(0x04, &val, 1); // AW88298_REG_SYSCTRL

  val = 0x0008; // RMSE=0 HAGCE=0 HDCCE=0 HMUTE=0
  this->write_bytes_16(0x05, &val, 1); //AW88298_REG_SYSCTRL2
  return true;
}

bool AW88298::apply_i2s_settings(const i2s_driver_config_t&  i2s_cfg){
  esph_log_d(TAG, "Setup AW88298");

  uint16_t val  = 0x4000; // I2SEN=1 AMPPD=0 PWDN=0
             val |= (1 << 6) ; // I2S Enable
             val |= (1 << 1) ; // power down amp for configuration
  this->write_bytes_16(0x04,  &val, 1);

  //sampling rate
  static constexpr uint8_t rate_tbl[] = {4,5,6,8,10,11,15,20,22,44};
  uint8_t rate_idx = 0;
  size_t rate = ( i2s_cfg.sample_rate + 1102) / 2205;
  while (rate > rate_tbl[rate_idx] && ++rate_idx < sizeof(rate_tbl)) {}

  //bits_per_sample
  static constexpr uint8_t supported_bps[] = {16,20,24,32};
  uint8_t bps = (uint8_t) i2s_cfg.bits_per_sample;
  uint8_t bps_idx = 0;
  while( bps != supported_bps[bps_idx] && ++bps_idx < sizeof(supported_bps)) {}
  if( bps_idx == sizeof(supported_bps))
  {
    esph_log_e(TAG, "Unsupported bits per sample: %d", bps);
    return false;
  }

  //bits_per_frame
  //00: 16*2 01: 24*2 10: 32*2 11:reserved
  uint8_t i2sbck = 0;

  size_t reg0x06_value = (1 << 12); //enable I2S receiver module
  reg0x06_value |=       (3 << 10); // mono, (L+R)/2
  reg0x06_value |= bps_idx  <<  6;
  reg0x06_value |= i2sbck   <<  4;
  reg0x06_value |= rate_idx;

  //reg0x06_value |= 0x1C00;  // I2SBCK=0 (BCK mode 16*2)
  this->write_bytes_16(0x06, (uint16_t *) &reg0x06_value, 1 );

  val  = 0x4000; // I2SEN=1 AMPPD=0 PWDN=0
             val |= (1 << 6) ; // I2S Enable
             val |= (0 << 1) ; // power up amp for configuration
  this->write_bytes_16(0x04,  &val, 1);
  return true;
}

bool AW88298::set_volume( float volume ){
  // 0 to 96 dB
  // 7:4 in unit of -6dB
  // 3:0 in unit of -0.5dB
  uint16_t val = (1. - volume) * 192.;
  val = (val / 12) << 4 | (val % 12);
  val = (val << 8 ) | 0x0064;
  this->write_bytes_16(0x0C, &val, 1 ); // AW88298_REG_HAGCCFG3
  return true;
}

bool AW88298::set_mute_audio( bool mute ){
  // Hardware mute module SYSCTRL2 (0x05) register bit 4
  uint16_t val;
  this->read_bytes_16(0x05, &val, 1);
  if( mute ){
    val |= (1 << 4);
  }
  else {
    val &= ~(1 << 4);
  }
  this->write_bytes_16(0x05, &val, 1);
  return true;
}

bool ES8388::init_device(){
  return true;
}


}
}

#endif
