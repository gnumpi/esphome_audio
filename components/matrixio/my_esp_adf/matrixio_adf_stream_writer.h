#pragma once

#ifdef USE_ESP_IDF 
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include "../matrixio.h"
#include "../wishbone.h"

#include "../../my_esp_adf/adf_audio_sinks.h"

#include <audio_element.h>
#include <audio_pipeline.h>


namespace esphome {
using namespace esp_adf;

namespace matrixio {

class MatrixIOStreamWriter : public WishboneDevice, public ADFPipelineSinkElement, public Component {
public:
  MatrixIOStreamWriter() : WishboneDevice(AUDIO_OUT_BASE_ADDRESS), 
                           output_selector_(OutputSelector::kHeadPhone), 
                           volume_(80) {}

  const std::string get_name() override {return "MatrixIO-Audio-Out";} 
  
  void setup() override;
  void dump_config() override;
  void loop() override;
  
  void mute()   {this->mute_();}
  void unmute() {this->unmute_();}
  void set_volume(float volume);
  void set_target_volume(int volume);
  void set_output(OutputSelector output_selector);
  float get_volume();
  void read_volume();

  bool set_pcm_sampling_frequency(uint32_t sampling_frequency);
  void set_sampling_frequency(int sampling_frequency);
  void set_number_of_channels(int channels );  
  
  uint32_t read_pcm_sampling_frequency();
  
  bool is_muted() const  {return this->mute_status_ == MuteStatus::kMute; }
  
  size_t write_to_fpga_fifo( uint8_t* buffer, int len );
  int channels{2};

  void on_settings_request(AudioPipelineSettingsRequest &request) override;
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