#pragma once

#ifdef USE_ESP_IDF

#include "../i2s_audio.h"

#include "esphome/core/component.h"

#include "../../adf_pipeline/adf_audio_sources.h"

namespace esphome {
using namespace esp_adf;
namespace i2s_audio {

class ADFElementI2SIn : public I2SReader, public ADFPipelineSourceElement, public Component {
 public:
  // ESPHome Component implementations
  void setup() override {}

  // ADFPipelieSourceElement implementations
  const std::string get_name() override { return "I2S_Reader"; }
  void dump_config() override { this->dump_i2s_settings(); }
  bool is_ready() override;

  protected:

  bool valid_settings_{false};
  bool init_adf_elements_() override;
  void clear_adf_elements_() override;
  audio_element_handle_t adf_i2s_stream_reader_;
};

}  // namespace i2s_audio
}  // namespace esphome
#endif
