#pragma once

#ifdef USE_ESP_IDF

#include "esphome/components/media_player/media_player.h"

#include "../adf_pipeline_controller.h"
#include "../adf_audio_sources.h"

namespace esphome {
namespace esp_adf {

class ADFPlaylistTrack {
  public:
    std::string uri{""};
    bool is_played{false};
};

class ADFMediaPlayer : public media_player::MediaPlayer, public ADFPipelineController {
 public:
  // Pipeline implementations
  void append_own_elements() { add_element_to_pipeline((ADFPipelineElement *) &(this->http_and_decoder_)); }

  // ESPHome-Component implementations
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }
  void setup() override;
  void dump_config() override;

  // MediaPlayer implementations
  bool is_muted() const override { return this->muted_; }
  media_player::MediaPlayerTraits get_traits() override;

  //
  void set_stream_uri(const std::string& new_uri);
  void start() {pipeline.start();}
  void stop()  {pipeline.stop();}

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
  //optional<std::string> current_uri_{};

  HTTPStreamReaderAndDecoder http_and_decoder_;

  std::vector<ADFPlaylistTrack > playlist_;
  bool playlist_found_{false};
  int play_track_id_{-1};
  
  void play_next_track_on_playlist_(int track_id);
  void set_decoder_type_(const std::string& uri);
  void clean_playlist_track_();
  int next_playlist_track_id_();
  int previous_playlist_track_id_();
  void set_playlist_track_as_played_(int track_id);
  int parse_m3u_into_playlist_(const char *url);
};

}  // namespace esp_adf
}  // namespace esphome

#endif
