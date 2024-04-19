#pragma once

#include "esphome/core/defines.h"

#define USE_ESP32_USB_STREAM
#ifdef USE_ESP32_USB_STREAM

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include <usb_stream.h>

namespace esphome {
namespace usb_audio {

class USBAudioComponent {
public:
  void setup_usb();

  void on_stream_state_changed(usb_stream_state_t event);
  void start_streaming();
  void stop_streaming();

protected:
  void read_frame_size_list();

  virtual void on_connected() {}
  virtual void on_disconnected() {}

  uint8_t  channels_{0};
  uint8_t  bits_per_sample_{0};
  uint16_t sample_rate_{0};

};

}
}
#endif
