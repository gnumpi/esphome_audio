#pragma once

#ifdef USE_ESP_IDF 

#include "esphome/components/media_player/media_player.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/helpers.h"

//#include "../adf_audio_element.h"
#include "../adf_pipeline.h"
#include "../adf_audio_sources.h"

#include "audio_pipeline.h"
#include "audio_element.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "http_stream.h"

#include <raw_stream.h>

namespace esphome {
namespace esp_adf {


class ADFMediaPlayer : public media_player::MediaPlayer, public ADFPipelineComponent {
 public:
  // Pipeline implementations
  void append_own_elements(){ add_element_to_pipeline( (ADFPipelineElement*) &(this->http_and_decoder_) ); }
  
  // ESPHome-Component implementations
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }
  void setup() override;
  void dump_config() override;  
 
  
  // MediaPlayer implementations
  bool is_muted() const override { return this->muted_; }
  media_player::MediaPlayerTraits get_traits() override;
  
  // 
  void set_stream_uri(const char *uri);
  void start(){}
  void stop(){}
  
protected:
  // MediaPlayer implementation
  void control(const media_player::MediaPlayerCall &call) override;

  // Pipeline implementations
  void on_pipeline_state_change(PipelineState state);

  void mute_();
  void unmute_();
  void set_volume_(float volume, bool publish = true);

  bool muted_{false};
  optional<std::string> current_url_{};
  
  HTTPStreamReaderAndDecoder http_and_decoder_;
};



}
}

#endif