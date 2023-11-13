#pragma once
#include <valarray>

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/microphone/microphone.h"
#include "esphome/components/matrixio/wishbone.h"

namespace esphome {
namespace matrixio {

// (sampling_rate, pdm_decimation, gain)
static const uint32_t MIC_SAMPLING_FREQUENCIES[][3] = {
    {8000, 374, 0},  {12000, 249, 2}, {16000, 186, 3}, {22050, 135, 5},
    {24000, 124, 5}, {32000, 92, 6},  {44100, 67, 7},  {48000, 61, 7},
    {96000, 30, 9},  {0, 0, 0}};

const uint32_t MICROPHONE_BASE_ADDRESS = 0x2000;
const uint16_t MICROPHONE_ARRAY_IRQ = 5;
const uint16_t NUMBER_OF_FIR_TAPS = 128;
const uint16_t MICROPHONE_CHANNELS = 8;

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
