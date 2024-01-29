#pragma once

#ifdef USE_ESP_IDF 
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "../wishbone.h"

#include "../../my_esp_adf/adf_audio_element.h"

#include <audio_element.h>
#include <audio_pipeline.h>


namespace esphome {
using namespace esp_adf;

namespace matrixio {

const uint16_t AUDIO_OUT_BASE_ADDRESS = 0x6000;
const uint16_t MAX_VOLUME_VALUE = 25;
const uint32_t FPGA_FIFO_SIZE = 4096;
const uint32_t MAX_WRITE_LENGTH = 1024;

const uint32_t PCM_SAMPLING_FREQUENCIES[][2] = {
    {8000, 975},  {16000, 492}, {32000, 245}, {44100, 177},
    {48000, 163}, {88200, 88},  {96000, 81},  {0, 0}};

enum MuteStatus : uint16_t {
  kMute = 0x0001,
  kUnMute = 0x0000
};

enum OutputSelector : uint16_t {
  kHeadPhone = 0x0001,
  kSpeaker = 0x0000
};

static const char *const TAG = "matrixio_adf_stream_writer";

class MatrixIOStreamWriter : public WishboneDevice, public ADFAudioComponent, public Component {
public:
  MatrixIOStreamWriter() : WishboneDevice(AUDIO_OUT_BASE_ADDRESS), 
                           output_selector_(OutputSelector::kHeadPhone), 
                           volume_(80) {}

  void setup() override;
  void dump_config() override;
  void loop() override;
  
  void mute()   override {this->mute_();}
  void unmute() override {this->unmute_();}
  void set_volume(float volume);
  void set_target_volume(int volume);
  void set_output(OutputSelector output_selector);
  float get_volume();
  void read_volume();

  void set_pcm_sampling_frequency(uint32_t sampling_frequency);
  void set_sampling_frequency(int sampling_frequency);
  void set_number_of_channels(int channels );  
  
  uint32_t read_pcm_sampling_frequency();
  
  bool hasInputBuffer()   const override {return true; }
  bool hasOutputBuffer()  const override {return false;}
  bool hasVolumeControl() const override {return true; }
  bool isMuteable()       const override {return true; }
  bool is_muted()         const override {return this->mute_status_ == MuteStatus::kMute; }
  
  size_t write_to_fpga_fifo( uint8_t* buffer, int len );
  int channels{2};

protected:
  void init_adf_elements_() override;
   
private:
  static void writing_task(void *params);
  uint16_t get_fpga_fifo_status_();
  
  void flush_fpga_fifo_();
  void write_output_select_();
  void write_volume_();
  void mute_();
  void unmute_();
  
  TaskHandle_t player_task_handle_{nullptr};
  QueueHandle_t buffer_queue_;
  QueueHandle_t event_queue_;
  
  uint32_t pcm_sampling_frequency_;
  OutputSelector output_selector_;
  MuteStatus mute_status_;
  Mutex lock_;
  
  float volume_;
  audio_element_handle_t audio_raw_stream_{nullptr};
};


#endif

}
}