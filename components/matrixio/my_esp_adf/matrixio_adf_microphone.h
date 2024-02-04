#pragma once

#ifdef USE_ESP_IDF 
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "../wishbone.h"

#include "../../my_esp_adf/adf_audio_sources.h"

#include <audio_element.h>
#include <audio_pipeline.h>


namespace esphome {
using namespace esp_adf;
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


class MatrixIOMicrophone : public WishboneDevice, public ADFPipelineSourceElement, public Component {
public:
  MatrixIOMicrophone() : WishboneDevice(MICROPHONE_BASE_ADDRESS) {} 
  const std::string get_name() override {return "MatrixIO-Microphones";}                     

  void setup();
  void dump_config() override;
  
  void set_sampling_rate(uint32_t sampling_rate);
  using WishboneDevice::wb_read;
protected:
  void init_adf_elements_() override;

private:
  void start_();
  void stop_();
  void read_();
  void write_fir_coeffs_();
  void write_sampling_rate_and_gain_();

  uint32_t sampling_rate_;
  uint16_t gain_;
  uint16_t pdm_decimation_;
  
  audio_element_handle_t audio_raw_stream_{nullptr};
};




}
}
#endif