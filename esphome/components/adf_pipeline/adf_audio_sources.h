#pragma once

#ifdef USE_ESP_IDF

#include "adf_audio_element.h"
#include <raw_stream.h>
namespace esphome {
namespace esp_adf {

class ADFPipelineSourceElement : public ADFPipelineElement {
 public:
  AudioPipelineElementType get_element_type() const { return AudioPipelineElementType::AUDIO_PIPELINE_SOURCE; }
};

class HTTPStreamReaderAndDecoder : public ADFPipelineSourceElement {
 public:
  void set_stream_uri(const char *uri);
  const std::string get_name() override { return "HTTPStreamReader"; }
  bool isReady() override;

 protected:
  void init_adf_elements_() override;
  void start_config_pipeline_();
  void terminate_config_pipeline_();
  void sdk_event_handler_(audio_event_iface_msg_t &msg);
  void cfg_event_handler_(audio_event_iface_msg_t &msg);


  bool waiting_for_cfg_{false};
  audio_element_handle_t http_stream_reader_{};
  audio_element_handle_t decoder_{};
  audio_pipeline_handle_t read_cfg_pipeline_{};
  audio_event_iface_handle_t cfg_pipeline_event_{};
};

class I2SReader : public ADFPipelineSourceElement {
 public:
  const std::string get_name() override { return "I2SReader"; }

 protected:
  void init_adf_elements_() override {}
  audio_element_handle_t adf_i2s_stream_reader_;
};

class PCMSource : public ADFPipelineSourceElement {
 public:
  const std::string get_name() override { return "PCMSource"; }
  int stream_write(char *buffer, int len);
  bool has_buffered_data() const;

 protected:
  void init_adf_elements_() override;
  audio_element_handle_t adf_raw_stream_writer_;
};

}  // namespace esp_adf
}  // namespace esphome
#endif
