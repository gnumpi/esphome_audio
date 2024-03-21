#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"

namespace esphome {
namespace audio_ic {

class AudioDecoderIF : public i2c::I2CDevice {
 public:
  virtual void setup_decoder() = 0;
  virtual void start() {}
  virtual void stop() {}

  virtual void set_sampling_rate() = 0;
  virtual void set_bits_per_sample() = 0;
  virtual void set_volume() = 0;
  virtual void mute() = 0;
  virtual void unmute() = 0;

 protected:
  virtual void write_parameter_() = 0;
  virtual void read_parameter_() = 0;

};

}
}
