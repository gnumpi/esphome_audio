#pragma once

#ifdef USE_ESP_IDF

#include "../../adf_pipeline/adf_pipeline_controller.h"
#include "bt_audio_in.h"

namespace esphome {
using namespace esp_adf;
namespace bt_audio {

class AdfBtPiepelineController : public ADFPipelineController {
 public:
  // Pipeline implementations
  void append_own_elements() { add_element_to_pipeline((ADFPipelineElement *) &(this->bt_audio_in_)); }

  // ESPHome-Component implementations
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }
  void setup() override;
  void dump_config() override {}

  void start() {pipeline.start();}
  void stop()  {pipeline.stop();}

 protected:

  // Pipeline implementations
  void on_pipeline_state_change(PipelineState state) {}

  ADFElementBTAudioIn bt_audio_in_;
};


}
}

#endif
