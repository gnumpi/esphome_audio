#include "adf_audio_process.h"


#ifdef USE_ESP_IDF
#include "adf_pipeline.h"

#include <filter_resample.h>

namespace esphome {
namespace esp_adf {

static const char *const TAG = "esp_audio_processors";

typedef struct rsp_filter {
    resample_info_t *resample_info;
    unsigned char *out_buf;
    unsigned char *in_buf;
    void *rsp_hd;
    int in_offset;
    int8_t flag;  //the flag must be 0 or 1. 1 means the parameter in `resample_info_t` changed. 0 means the parameter in `resample_info_t` is original.
} rsp_filter_t;


bool ADFResampler::init_adf_elements_(){
    rsp_filter_cfg_t rsp_cfg = {
      .src_rate = this->src_rate_,
      .src_ch = this->src_num_channels_,
      .dest_rate = this->dst_rate_,
      .dest_bits = 16,
      .dest_ch = this->dst_num_channels_,
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
  bool settings_changed = false;
  if( request.sampling_rate > -1 ){
    if( request.sampling_rate != this->src_rate_ )
    {
      this->src_rate_ = request.sampling_rate;
      settings_changed = true;
    }
    uint32_t dst_rate = request.final_sampling_rate > -1 ? request.final_sampling_rate : this->src_rate_;
    if( dst_rate != this->dst_rate_ )
    {
      this->dst_rate_ = dst_rate;
      settings_changed = true;
    }
  }
  if( request.number_of_channels > -1 ){
    if( request.number_of_channels != this->src_num_channels_ )
    {
      this->src_num_channels_ = request.number_of_channels;
      settings_changed = true;
    }
    uint32_t dst_num_channels = request.final_number_of_channels > -1 ? request.final_number_of_channels : this->src_num_channels_;
    if( dst_num_channels != this->dst_num_channels_ )
    {
      this->dst_num_channels_ = dst_num_channels;
      settings_changed = true;
    }
  }
  esph_log_d(TAG, "New settings: SRC: rate: %d, ch: %d DST: rate: %d, ch: %d ", this->src_rate_,this->src_num_channels_, this->dst_rate_, this->dst_num_channels_);

  if( this->sdk_resampler_ && settings_changed)
  {
    if( audio_element_get_state(this->sdk_resampler_) == AEL_STATE_RUNNING)
    {
      audio_element_stop(this->sdk_resampler_);
      audio_element_wait_for_stop(this->sdk_resampler_);
    }
    esph_log_d(TAG, "New settings: SRC: rate: %d, ch: %d DST: rate: %d, ch: %d ", this->src_rate_, this->src_num_channels_, this->dst_rate_, this->dst_num_channels_);
    rsp_filter_t *filter = (rsp_filter_t *)audio_element_getdata(this->sdk_resampler_);
    resample_info_t &resample_info = *(filter->resample_info);
    resample_info.src_rate = this->src_rate_;
    resample_info.dest_rate = this->dst_rate_;
    resample_info.src_ch = this->src_num_channels_;
    resample_info.dest_ch = this->dst_num_channels_;
  }
}

}  // namespace esp_adf
}  // namespace esphome

#endif
