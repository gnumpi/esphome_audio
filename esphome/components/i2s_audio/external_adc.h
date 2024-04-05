#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/gpio.h"

#ifdef I2S_EXTERNAL_ADC

#include "esphome/components/i2c/i2c.h"
#include <driver/i2s.h>

namespace esphome {
namespace i2s_audio {


class ExternalADC : public i2c::I2CDevice {
public:
  virtual bool init_device() = 0;
  virtual bool deinit_device() {return true;}

  virtual bool apply_i2s_settings(const i2s_driver_config_t&  i2s_cfg) {return true;}

  void set_gpio_enable(GPIOPin* enable_pin){this->enable_pin_ = enable_pin;}
protected:
  GPIOPin* enable_pin_{nullptr};
};

class ES7210 : public ExternalADC {
public:
  bool init_device() override;
  bool apply_i2s_settings(const i2s_driver_config_t&  i2s_cfg) override;
};

}
}

#endif
