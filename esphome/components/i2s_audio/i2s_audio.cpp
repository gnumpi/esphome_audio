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
  if( this->current_mode_ == 0 ) {
    this->current_mode_ = mode;
    success = true;
  }
  else if( this->current_mode_ == mode ){
    success = true;
  }
  this->unlock();
  return success;
  }

bool I2SAudioComponent::release_mode_(int mode){
  bool success = false;
  this->lock();
  esph_log_d(TAG, "Release I2S Lock Mode: %d", mode);
  if( this->current_mode_ == mode ) {
    this->current_mode_ = 0;
    success = true;
  } else if( this->current_mode_ == 0 )
  {
    success = true;
  }
  this->unlock();
  return success;
}



}  // namespace i2s_audio
}  // namespace esphome

#endif  // USE_ESP32
