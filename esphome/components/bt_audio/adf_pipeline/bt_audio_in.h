#pragma once

#include "esphome/core/defines.h"

#define USE_ESP32_BT
#ifdef USE_ESP32_BT

#include "../bt_audio.h"

#include "esphome/core/component.h"

#include "../../adf_pipeline/adf_audio_sources.h"

namespace esphome {
using namespace esp_adf;
namespace bt_audio {

class ADFElementBTAudioIn : public BTAudioComponent, public ADFPipelineSourceElement, public Component {
public:
  void setup() override {}
  const std::string get_name() override { return "BT_AUDIO_IN"; }
  void dump_config() override {}
  bool is_ready() override {return this->is_connected();}

protected:
  bool init_adf_elements_() override;
  audio_element_handle_t adf_bt_stream_reader_;
};

}
}
#endif
