#pragma once

#ifdef USE_ESP_IDF

#include "esphome/components/microphone/microphone.h"

#include "../adf_pipeline_controller.h"
#include "../adf_audio_sinks.h"

namespace esphome {
namespace esp_adf {

class ADFMicrophone : public microphone::Microphone, public ADFPipelineController {
 public:
  // Pipeline implementations
  void append_own_elements() { add_element_to_pipeline((ADFPipelineElement *) &(this->pcm_stream_)); }

  // ESPHome-Component implementations
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }
  void setup() override;
  void dump_config() override;

  // Microphone implementation
  void start() override;
  void stop() override;
  size_t read(int16_t *buf, size_t len) override;

 protected:
  uint8_t gain_log_2_{0};
  void on_pipeline_state_change(PipelineState state) override;
  PCMSink pcm_stream_;
};

}  // namespace esp_adf
}  // namespace esphome

#endif
