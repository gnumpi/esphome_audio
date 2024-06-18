#pragma once

#ifdef USE_ESP_IDF

#include "esphome/components/media_player/media_player.h"

#include "../adf_pipeline_controller.h"
#include "../adf_audio_sources.h"

namespace esphome {
namespace esp_adf {


class ADFMediaPlayer : public media_player::MediaPlayer, public ADFPipelineController {
 public:
  // Pipeline implementations
  void append_own_elements() { add_element_to_pipeline((ADFPipelineElement *) &(this->http_and_decoder_)); }
  const std::string get_name() {return "MediaPlayer";}

  // ESPHome-Component implementations
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }
  void setup() override;
  void dump_config() override;
  void loop() override;

  // MediaPlayer implementations
  bool is_muted() const override { return this->muted_; }
  media_player::MediaPlayerTraits get_traits() override;

  //
  void set_stream_uri(const std::string& new_uri);
  void set_announcement_uri(const std::string& new_uri);

  void set_announce_base_track(ADFCodec codec, int rate, int bits, int channels){
    this->announce_base_track_ = Track(codec, rate, bits, channels);
  }

  void set_announce_track(const Track track){ this->announce_track_ = track; }
  void set_current_track(const Track track){ this->current_track_ = track; }
  void set_next_track(const Track track){ this->next_track_ = track; }

  void start() {pipeline.start();}
  void stop()  {pipeline.stop();}

  bool set_new_state(media_player::MediaPlayerState new_state){
    if( this->state == new_state ){
      return false;
    }
    this->state = new_state;
    return true;
  }
  protected:
  // MediaPlayer implementation
  void control(const media_player::MediaPlayerCall &call) override;

  // Pipeline implementations
  void on_pipeline_state_change(PipelineState state);

  void mute_();
  void unmute_();
  void set_volume_(float volume, bool publish = true);

  bool muted_{false};
  bool play_intent_{false};
  bool announcement_{false};
  optional<std::string> current_uri_{};
  optional<std::string> announcement_uri_{};

  Track announce_base_track_{};
  optional<Track> announce_track_{};
  optional<Track> current_track_{};
  optional<Track> next_track_{};

  HTTPStreamReaderAndDecoder* curr_player_{};
  HTTPStreamReaderAndDecoder* next_player_{};

  HTTPStreamReaderAndDecoder http_and_decoder_;
};

}  // namespace esp_adf
}  // namespace esphome

#endif
