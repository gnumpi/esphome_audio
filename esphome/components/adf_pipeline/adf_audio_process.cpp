#include "adf_audio_process.h"


#ifdef USE_ESP_IDF
#include "adf_pipeline.h"

#include <filter_resample.h>


namespace esphome {
namespace esp_adf {

static const char *const TAG = "esp_audio_processors";

bool ADFResampler::init_adf_elements_(){
    rsp_filter_cfg_t rsp_cfg = {
      .src_rate = 44100,
      .src_ch = 2,
      .dest_rate = 48000,
      .dest_bits = 16,
      .dest_ch = 2,
      .src_bits = 16,
      .mode = RESAMPLE_DECODE_MODE,
      .max_indata_bytes = RSP_FILTER_BUFFER_BYTE,
      .out_len_bytes = RSP_FILTER_BUFFER_BYTE,
      .type = ESP_RESAMPLE_TYPE_AUTO,
      .complexity = 2,
      .down_ch_idx = 0,
      .prefer_flag = ESP_RSP_PREFER_TYPE_SPEED,
      .out_rb_size = RSP_FILTER_RINGBUFFER_SIZE,
      .task_stack = RSP_FILTER_TASK_STACK,
      .task_core = RSP_FILTER_TASK_CORE,
      .task_prio = RSP_FILTER_TASK_PRIO,
      .stack_in_ext = true,
  };
  this->sdk_resampler_= rsp_filter_init(&rsp_cfg);
  sdk_audio_elements_.push_back(this->sdk_resampler_);
  sdk_element_tags_.push_back("resampler");
  return true;
}

void ADFResampler::on_settings_request(AudioPipelineSettingsRequest &request){

}

}  // namespace esp_adf
}  // namespace esphome

#endif
