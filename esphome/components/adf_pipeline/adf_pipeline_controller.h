#pragma once

#include <cstring>
#include <vector>
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF

#include "adf_pipeline.h"

namespace esphome {
namespace esp_adf {

class ADFPipelineElement;

/*
An ESPHome Component for managing an ADFPipeline
*/
class ADFPipelineController : public Component {
public:
  ADFPipelineController() : pipeline(this) {}
  ~ADFPipelineController() {}

  virtual void append_own_elements() {}
  void add_element_to_pipeline(ADFPipelineElement *element) { pipeline.append_element(element); }
  void set_keep_alive(bool value) { this->pipeline.set_destroy_on_stop(!value); }

  void setup() override {}
  void dump_config() override { pipeline.dump_element_configs(); };
  void loop() override { pipeline.loop(); }

protected:
  friend ADFPipeline;
  virtual void pipeline_event_handler(audio_event_iface_msg_t &msg) {}
  virtual void on_pipeline_state_change(PipelineState state) {}

  ADFPipeline pipeline;
};


}
}

#endif
