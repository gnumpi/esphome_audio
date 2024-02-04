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
  adf_raw_stream_reader_ = raw_stream_init(&raw_cfg);
}

int PCMSink::stream_read(char* buffer, int len){
  return raw_stream_read(adf_raw_stream_reader_, buffer, len);
}



}
}

#endif