#pragma once

#ifdef USE_ESP32

#include <driver/i2s.h>
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace i2s_audio {

enum class I2SAccessMode  : uint8_t {EXCLUSIVE, DUPLEX};
class I2SAccess {
public:
   static constexpr uint8_t FREE = 0;
   static constexpr uint8_t RX   = 1;
   static constexpr uint8_t TX   = 2;
};


class I2SAudioIn;
class I2SAudioOut;
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

  void lock() { this->lock_.lock(); }
  bool try_lock() { return this->lock_.try_lock(); }
  void unlock() { this->lock_.unlock(); }

  i2s_port_t get_port() const { return this->port_; }
  void set_audio_in(I2SAudioIn* comp_in){ this->audio_in_ = comp_in;}
  void set_audio_out(I2SAudioOut* comp_out){ this->audio_out_ = comp_out;}

  void set_access_mode(I2SAccessMode access_mode){this->access_mode_ = access_mode;}
  bool adjustable(){return this->access_mode_ == I2SAccessMode::EXCLUSIVE;}

 protected:
  friend I2SAudioIn;
  friend I2SAudioOut;

  Mutex lock_;
  I2SAccessMode access_mode_{I2SAccessMode::DUPLEX};
  uint8_t access_state_{I2SAccess::FREE};

  bool set_access_(uint8_t access);
  bool release_access_(uint8_t access);
  bool install_i2s_driver_(i2s_driver_config_t i2s_cfg, uint8_t access);
  bool uninstall_i2s_driver_(uint8_t access);
  bool validate_cfg_for_duplex_(i2s_driver_config_t& i2s_cfg);

  I2SAudioIn *audio_in_{nullptr};
  I2SAudioOut *audio_out_{nullptr};

  int mclk_pin_{I2S_PIN_NO_CHANGE};
  int bclk_pin_{I2S_PIN_NO_CHANGE};
  int lrclk_pin_;
  i2s_port_t port_{};

  i2s_driver_config_t installed_cfg_{};
};

class ExternalADC;
class ExternalDAC;


class I2SAudioIn : public Parented<I2SAudioComponent> {
public:
   bool install_i2s_driver(i2s_driver_config_t i2s_cfg){
      return this->parent_->install_i2s_driver_(i2s_cfg, I2SAccess::RX);}
   bool uinstall_i2s_driver(){ return this->parent_->uninstall_i2s_driver_(I2SAccess::RX);}
   bool set_i2s_access(){return this->parent_->set_access_(I2SAccess::RX);};
   bool release_i2s_access(){return this->parent_->release_access_(I2SAccess::RX);};

   void set_external_adc(ExternalADC* adc){this->external_adc_ = adc;}
   void set_din_pin(int8_t pin) { this->din_pin_ = pin; }
   int8_t get_din_pin() { return this->din_pin_; }

protected:
   ExternalADC* external_adc_{};
   int8_t din_pin_{I2S_PIN_NO_CHANGE};
};

class I2SAudioOut : public Parented<I2SAudioComponent> {
public:
   bool install_i2s_driver(i2s_driver_config_t i2s_cfg){
      return this->parent_->install_i2s_driver_(i2s_cfg, I2SAccess::TX); }
   bool uinstall_i2s_driver(){ return this->parent_->uninstall_i2s_driver_(I2SAccess::RX);}
   bool set_i2s_access(){return this->parent_->set_access_(I2SAccess::TX);};
   bool release_i2s_access(){return this->parent_->release_access_(I2SAccess::TX);};

   void set_external_dac(ExternalDAC* dac){this->external_dac_ = dac;}
   void set_dout_pin(int8_t pin) { this->dout_pin_ = pin; }
   int8_t get_dout_pin() { return this->dout_pin_; }

protected:
   ExternalDAC* external_dac_{};
   int8_t dout_pin_{I2S_PIN_NO_CHANGE};
};




}  // namespace i2s_audio
}  // namespace esphome

#endif  // USE_ESP32
