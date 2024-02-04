#pragma once

#ifdef USE_ESP_IDF 
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include "../matrixio.h"
#include "../wishbone.h"

#include "../../my_esp_adf/adf_audio_sources.h"

#include <audio_element.h>
#include <audio_pipeline.h>


namespace esphome {
using namespace esp_adf;
namespace matrixio {


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