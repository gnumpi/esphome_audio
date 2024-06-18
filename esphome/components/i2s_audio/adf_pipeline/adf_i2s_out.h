#pragma once

#ifdef USE_ESP_IDF

#include "../i2s_audio.h"

#include "esphome/core/component.h"

#include "../../adf_pipeline/adf_audio_sinks.h"

namespace esphome {
using namespace esp_adf;
namespace i2s_audio {

class ADFElementI2SOut : public I2SWriter, public ADFPipelineSinkElement, public Component {
 public:
  // ESPHome Component implementations
  void setup() override;

  // ADFPipelieSourceElement implementations
  const std::string get_name() override { return "I2S_Writer"; }
  void dump_config() override { this->dump_i2s_settings(); }
  bool is_ready() override;

  void set_use_adf_alc(bool use_alc){ this->use_adf_alc_ = use_alc; }
  void set_alc_max_val(float max_val){ this->max_alc_val_ = max_val;}
  void set_finish_on_timeout_ms(int timeout){this->finish_on_timeout_ms_ = timeout;}
 protected:
  void on_settings_request(AudioPipelineSettingsRequest &request) override;
  bool use_adf_alc_{false};
  float max_alc_val_{1.};
  bool adjustable_{false};
  int finish_on_timeout_ms_{0};

  bool init_adf_elements_() override;
  void clear_adf_elements_() override;

  audio_element_handle_t adf_i2s_stream_writer_;
};

}  // namespace i2s_audio
}  // namespace esphome
#endif
