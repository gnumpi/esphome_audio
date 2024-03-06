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
  void set_stream_uri(const std::string&  new_url);
  const std::string get_name() override { return "HTTPStreamReader"; }
  bool is_ready() override;
  void prepare_elements() override;

 protected:
  bool init_adf_elements_() override;
  void clear_adf_elements_() override;
  void reset_() override;

  void start_prepare_pipeline_();
  void terminate_prepare_pipeline_();
  bool set_ready_when_prepare_pipeline_stopped_();
  void sdk_event_handler_(audio_event_iface_msg_t &msg);
  void cfg_event_handler_(audio_event_iface_msg_t &msg);


  PipelineElementState element_state_{PipelineElementState::UNINITIALIZED};
  std::string current_url_{"https://dl.espressif.com/dl/audio/ff-16b-2c-44100hz.mp3"};
  audio_element_handle_t http_stream_reader_{};
  audio_element_handle_t decoder_{};
};


class PCMSource : public ADFPipelineSourceElement {
 public:
  const std::string get_name() override { return "PCMSource"; }
  int stream_write(char *buffer, int len);
  bool has_buffered_data() const;

 protected:
  bool init_adf_elements_() override;
  audio_element_handle_t adf_raw_stream_writer_;
};

}  // namespace esp_adf
}  // namespace esphome
#endif
