#pragma once

#include <valarray>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/speaker/speaker.h"

#include "../matrixio.h"
#include "../wishbone.h"

namespace esphome {
namespace matrixio {


enum class TaskEventType : uint8_t {
  STARTING = 0,
  STARTED,
  PLAYING,
  STOPPING,
  STOPPED,
  WARNING = 255,
};

struct TaskEvent {
  TaskEventType type;
  esp_err_t err;
};

struct DataEvent {
  bool stop;
  size_t len;
  uint8_t data[BUFFER_SIZE];
};


static const char *const TAG = "matrixio_speaker";
class Speaker : public speaker::Speaker, public matrixio::WishboneDevice, public Component {
public:
  Speaker() : matrixio::WishboneDevice(AUDIO_OUT_BASE_ADDRESS), output_(OutputSelector::kHeadPhone), volume_(80) {}

  void setup() override;
  void dump_config() override;

  void start() override;
  void stop() override;
  void loop() override;

  size_t play(const uint8_t *data, size_t length) override;
  void mute();
  void unmute();
  void set_volume(uint8_t volume_percentage);
  void set_output(OutputSelector output_selector);
  uint8_t get_volume();

  void set_pcm_sampling_frequency(uint32_t sampling_frequency);
  uint32_t read_pcm_sampling_frequency();

  bool has_buffered_data() const override;

private:
  void start_();
  void watch_();

  static void player_task(void *params);

  uint16_t get_fpga_fifo_status_();
  void flush_fpga_fifo_();
  void write_output_();
  void write_volume_();

  TaskHandle_t player_task_handle_{nullptr};
  QueueHandle_t buffer_queue_;
  QueueHandle_t event_queue_;
  uint16_t buffer[BUFFER_SIZE] = {0};

  uint32_t pcm_sampling_frequency_;
  OutputSelector output_;
  MuteStatus mute_status_;
  uint8_t volume_;
};

}
}
