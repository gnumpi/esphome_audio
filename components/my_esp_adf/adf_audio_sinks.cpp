#include "adf_audio_sinks.h"
#include "adf_pipeline.h"

#ifdef USE_ESP_IDF 

#include <raw_stream.h>

namespace esphome {
namespace esp_adf {


void PCMSink::init_adf_elements_(){
  raw_stream_cfg_t raw_cfg = {
      .type = AUDIO_STREAM_READER,
      .out_rb_size = 8 * 1024,
  };
  this->adf_raw_stream_reader_ = raw_stream_init(&raw_cfg);
  audio_element_set_input_timeout(this->adf_raw_stream_reader_, 10 / portTICK_RATE_MS);
  this->sdk_audio_elements_.push_back(this->adf_raw_stream_reader_);
}

int PCMSink::stream_read(char* buffer, int len){
  int ret = audio_element_input(adf_raw_stream_reader_, buffer, len);
  if ( ret < 0 && (ret != AEL_IO_TIMEOUT) ){
    audio_element_report_status(adf_raw_stream_reader_, AEL_STATUS_STATE_STOPPED);
  } else if(ret < 0 ){
    return 0;
  }
  return ret;
}



}
}

#endif