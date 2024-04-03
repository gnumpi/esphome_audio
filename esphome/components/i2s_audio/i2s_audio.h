#pragma once

#ifdef USE_ESP32

#include <driver/i2s.h>
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace i2s_audio {

enum class I2SAccessMode  : uint8_t {EXCLUSIVE_TX = 0, EXCLUSIVE_RX, DUPLEX};
enum class I2SAccessState : uint8_t {FREE = 0, EXCLUSIVE_TX, EXCLUSIVE_RX, DUPLEX};

class ExternalADC;
class ExternalDAC;
class I2SAudioComponent;

class I2SAudioIn : public Parented<I2SAudioComponent> {
public:
   void set_external_adc(ExternalADC* adc){this->external_adc_ = adc;}
   void set_dout_pin(uint8_t pin) { this->din_pin_ = pin; }
   uint8_t get_din_pin() { return this->din_pin_; }

protected:
   ExternalADC* external_adc_{};
   uint8_t din_pin_{0};
};

class I2SAudioOut : public Parented<I2SAudioComponent> {
public:
   void set_external_dac(ExternalDAC* dac){this->external_dac_ = dac;}
   void set_dout_pin(uint8_t pin) { this->dout_pin_ = pin; }
   uint8_t get_dout_pin() { return this->dout_pin_; }

protected:
   ExternalDAC* external_dac_{};
   uint8_t dout_pin_{0};
};


class I2SAudioComponent : public Component {
 public:
  void setup() override;

  i2s_pin_config_t get_pin_config() const {
    return {
        .mck_io_num = this->mclk_pin_,
        .bck_io_num = this->bclk_pin_,
        .ws_io_num = this->lrclk_pin_,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_PIN_NO_CHANGE,
    };
  }

  i2s_driver_config_t get_i2s_cfg() const;

  void set_mclk_pin(int pin) { this->mclk_pin_ = pin; }
  void set_bclk_pin(int pin) { this->bclk_pin_ = pin; }
  void set_lrclk_pin(int pin) { this->lrclk_pin_ = pin; }

  //depricated:
  bool set_read_mode() { return !this->exclusive_mode_ || set_mode_(1); }
  bool set_write_mode(){ return !this->exclusive_mode_ || set_mode_(2); }
  bool release_read_mode(){ return !this->exclusive_mode_ || release_mode_(1); }
  bool release_write_mode(){ return !this->exclusive_mode_ || release_mode_(2); }

  void lock() { this->lock_.lock(); }
  bool try_lock() { return this->lock_.try_lock(); }
  void unlock() { this->lock_.unlock(); }

  bool install_i2s_driver();
  bool uninstall_i2s_driver();

  i2s_port_t get_port() const { return this->port_; }
  bool dynamic_i2s_settings{false};

 protected:
  Mutex lock_;
  I2SAccessMode access_mode_{I2SAccessMode::DUPLEX};
  I2SAccessState access_state_{I2SAccessState::FREE};
  bool exclusive_mode_{true};
  bool set_mode_(int mode );
  bool release_mode_(int mode);

  I2SAudioIn *audio_in_{nullptr};
  I2SAudioOut *audio_out_{nullptr};

  int mclk_pin_{I2S_PIN_NO_CHANGE};
  int bclk_pin_{I2S_PIN_NO_CHANGE};
  int lrclk_pin_;
  i2s_port_t port_{};

  uint32_t sample_rate_{48000};

};

}  // namespace i2s_audio
}  // namespace esphome

#endif  // USE_ESP32
