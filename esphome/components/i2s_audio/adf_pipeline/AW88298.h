#pragma once

#include "esphome/core/defines.h"

#ifdef ADF_PIPELINE_I2C_IC
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"

#include "adf_i2s_out.h"

namespace esphome {
using namespace esp_adf;
namespace i2s_audio {

class ADFI2SOut_AW88298 : public ADFElementI2SOut, public i2c::I2CDevice {
 public:
  void setup() override;

  float get_setup_priority() const override { return setup_priority::LATE - 1; }
  void on_pipeline_status_change() override;

protected:
   void on_settings_request(AudioPipelineSettingsRequest &request) override;
};

}  // namespace esp_adf
}  // namespace esphome

#endif
