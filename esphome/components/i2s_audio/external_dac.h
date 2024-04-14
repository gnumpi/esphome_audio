#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/gpio.h"

#ifdef I2S_EXTERNAL_DAC

#include "esphome/components/i2c/i2c.h"
#include <driver/i2s.h>

namespace esphome {
namespace i2s_audio {


class ExternalDAC : public i2c::I2CDevice {
public:
  virtual bool init_device() = 0;
  virtual bool deinit_device() {return true;}

  virtual bool apply_i2s_settings(const i2s_driver_config_t&  i2s_cfg) {return true;}
  virtual bool set_mute_audio( bool mute ){return true;}
  virtual bool set_volume( float volume ){return true;}

  void set_gpio_enable(GPIOPin* enable_pin){this->enable_pin_ = enable_pin;}
protected:
  GPIOPin* enable_pin_{nullptr};
};

class AW88298 : public ExternalDAC {
  bool init_device() override;
  bool apply_i2s_settings(const i2s_driver_config_t&  i2s_cfg) override;
  bool set_mute_audio( bool mute );
  bool set_volume( float volume );
};

class ES8388 : public ExternalDAC {
  bool init_device() override;
  bool apply_i2s_settings(const i2s_driver_config_t&  i2s_cfg) override;
  bool set_mute_audio( bool mute );
};


}
}
#endif
