#pragma once

#ifdef USE_ESP_IDF

#include "../adf_pipeline.h"
#include "../adf_audio_sinks.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "esphome/components/microphone/microphone.h"

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include <audio_element.h>
#include <audio_pipeline.h>

namespace esphome {
namespace esp_adf {

class ADFMicrophone : public microphone::Microphone, public ADFPipelineComponent {
public:
  // Pipeline implementations
  void append_own_elements(){ add_element_to_pipeline( (ADFPipelineElement*) &(this->pcm_stream_) ); }
  
  // ESPHome-Component implementations
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }
  void setup() override;
  void loop() override;
  void dump_config() override;

  //Microphone implementation
  void start() override;
  void stop() override;
  size_t read(int16_t *buf, size_t len) override;

protected:
  void on_pipeline_state_change(PipelineState state) override;
  PCMSink pcm_stream_;
};

}
}
#endif