#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"

namespace esphome {
namespace aw88298 {

class AW88298Component : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void setup_aw88298();

  float get_setup_priority() const override { return setup_priority::LATE - 1; }
};

}  // namespace aw88298
}  // namespace esphome
