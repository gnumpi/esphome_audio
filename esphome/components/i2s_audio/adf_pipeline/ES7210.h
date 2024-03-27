#pragma once

#include "esphome/core/defines.h"

#ifdef ADF_PIPELINE_I2C_IC

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"

#include "adf_i2s_in.h"

namespace esphome {
using namespace esp_adf;
namespace i2s_audio {


class ADFI2SIn_ES7210 : public ADFElementI2SIn, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::LATE - 1; }
  void on_pipeline_status_change() override {}

protected:
  bool setup_es7210_(){return true;}

  void sdk_event_handler_(audio_event_iface_msg_t &msg){}
  void on_settings_request(AudioPipelineSettingsRequest &request) override{}
};

}  // namespace esp_adf
}  // namespace esphome

#endif
