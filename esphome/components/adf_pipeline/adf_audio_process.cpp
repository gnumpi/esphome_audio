#include "adf_audio_process.h"
#include "adf_pipeline.h"

#ifdef USE_ESP_IDF


namespace esphome {
namespace esp_adf {

bool ADFResampler::validate_settings_(){
    return true;
}

void ADFResampler::init_adf_elements_(){
    this->validate_settings_();

    rsp_filter_cfg_t rsp_cfg = {0};
    rsp_cfg.src_rate = this->format_in_.rate;
    rsp_cfg.src_ch = this->format_in_.channels;
    rsp_cfg.src_bits = this->format_in_.bits;

    rsp_cfg.dest_rate = this->format_out_.rate;
    rsp_cfg.dest_ch = this->format_out_.channels;
    rsp_cfg.dest_bits = this->format_out_.bits;
    rsp_cfg.down_ch_idx = 0;

    rsp_cfg.mode = this->mode_;
    rsp_cfg.max_indata_bytes = RSP_FILTER_BUFFER_BYTE;
    rsp_cfg.out_len_bytes = RSP_FILTER_BUFFER_BYTE;
    rsp_cfg.type = ESP_RESAMPLE_TYPE_AUTO;
    rsp_cfg.complexity = 0;

    rsp_cfg.prefer_flag = ESP_RSP_PREFER_TYPE_SPEED;
    rsp_cfg.out_rb_size = (2 * 1024);
    rsp_cfg.task_stack = RSP_FILTER_TASK_STACK;
    rsp_cfg.task_core = RSP_FILTER_TASK_CORE;
    rsp_cfg.task_prio = RSP_FILTER_TASK_PRIO;
    rsp_cfg.stack_in_ext = false;

    this->adf_resample_filter_ = rsp_filter_init(&rsp_cfg);
}

}
}

#endif
