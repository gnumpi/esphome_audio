#pragma once

#include "esphome/core/defines.h"

#define USE_ESP32_BT
#ifdef USE_ESP32_BT

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_peripherals.h"

namespace esphome {
namespace bt_audio {

class BTAudioComponent {
public:
  void setup_bt();

  bool try_to_connect() {return true;}
  bool is_connected() {return true; }

protected:
  virtual void on_connection_timeout() {}

  std::string bt_dev_name_{};
  esp_periph_handle_t bt_periph_{nullptr};
};

}
}
#endif
