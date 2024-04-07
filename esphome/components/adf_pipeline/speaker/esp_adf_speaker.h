#pragma once

#ifdef USE_ESP_IDF

#include "esphome/components/speaker/speaker.h"

#include "../adf_pipeline_controller.h"
#include "../adf_audio_sources.h"

namespace esphome {
namespace esp_adf {

static const size_t BUFFER_SIZE = 1024;

class ADFSpeaker : public speaker::Speaker, public ADFPipelineController {
 public:
  // Pipeline implementations
  void append_own_elements(){ add_element_to_pipeline( (ADFPipelineElement*) &(this->pcm_stream_) ); }

  // ESPHome-Component implementations
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }
  void  setup() override;
  void  dump_config() override;
  void  loop() override;


  // Speaker implemenations
  void start() override;
  void stop() override;
  size_t play(const uint8_t *data, size_t length) override;
  bool has_buffered_data() const override;

 protected:
  void start_();
  void watch_();

  // Pipeline implementations
  void on_pipeline_state_change(PipelineState state) override;
  void request_pipeline_settings_();
  PCMSource pcm_stream_;
};

}  // namespace esp_adf
}  // namespace esphome

#endif  // USE_ESP_IDF
