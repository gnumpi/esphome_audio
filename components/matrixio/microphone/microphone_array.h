#pragma once
#include <valarray>

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/microphone/microphone.h"

#include "../matrixio.h"
#include "../wishbone.h"

namespace esphome {
namespace matrixio {


class Microphone : public microphone::Microphone, public WishboneDevice, public Component {

public:
  Microphone() : WishboneDevice(MICROPHONE_BASE_ADDRESS) {}

  void set_sampling_rate(uint32_t sampling_rate);
  void setup() override;
  void dump_config() override;
  void write_config();

  void start() override;
  void stop() override;
  void loop() override;

  size_t read(int16_t *buf, size_t len) override;

private:
  void start_();
  void stop_();
  void read_();
  void write_fir_coeffs();
  void write_sampling_rate_and_gain();

  uint32_t sampling_rate_;
  uint16_t gain_;
  uint16_t pdm_decimation_;
  HighFrequencyLoopRequester high_freq_;
};


}
}
