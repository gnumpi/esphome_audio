#pragma once

#ifdef USE_ESP_IDF

#include "esphome/components/media_player/media_player.h"

#include "../adf_pipeline_controller.h"
#include "../adf_audio_sources.h"

namespace esphome {
namespace esp_adf {

class ADFPlaylistTrack {
  public:
    unsigned int order{0};
    std::string uri{""};
    bool is_played{false};
    std::string artist{""};
    std::string album{""};
    std::string title{""};
    
    // Overloading < operator 
    bool operator<(const ADFPlaylistTrack& obj) const
    { 
        return order < obj.order; 
    } 
};

class ADFMediaPlayer : public media_player::MediaPlayer, public ADFPipelineController {
 public:
  // Pipeline implementations
  void append_own_elements() { add_element_to_pipeline((ADFPipelineElement *) &(this->http_and_decoder_)); }

  // ESPHome-Component implementations
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }
  void setup() override;
  void dump_config() override;
  void publish_state();
  media_player::MediaPlayerState prior_state{media_player::MEDIA_PLAYER_STATE_NONE};

  // MediaPlayer implementations
  bool is_muted() const override { return this->muted_; }
  std::string repeat() const { return this->repeat_; }
  bool is_shuffle() const override { return this->shuffle_; }
  std::string artist() const { return this->artist_; }
  std::string album() const { return this->album_; }
  std::string title() const { return this->title_; }
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
  void set_repeat_(const std::string& repeat);
  void set_shuffle_(bool shuffle);
  void set_artist_(const std::string& artist) {artist_ = artist;}
  void set_album_(const std::string& album) {album_ = album;}
  void set_title_(const std::string& title) {title_ = title;}
  void set_volume_(float volume, bool publish = true);

  bool muted_{false};
  std::string repeat_{"off"};
  bool shuffle_{false};
  std::string artist_{""};
  std::string album_{""};
  std::string title_{""};
  bool play_intent_{false};
  bool turning_off_{false};
  std::string current_uri_{};
  bool force_publish_{false};

  HTTPStreamReaderAndDecoder http_and_decoder_;

  std::vector<ADFPlaylistTrack > playlist_;
  bool playlist_found_{false};
  int play_track_id_{-1};
  
  void play_next_track_on_playlist_(int track_id);
  void clean_playlist_track_();
  int next_playlist_track_id_();
  int previous_playlist_track_id_();
  void set_playlist_track_as_played_(int track_id);
  void playlist_add_(const std::string& new_uri);
  int parse_m3u_into_playlist_(const char *url);
  void set_playlist_track_(ADFPlaylistTrack track);
};

}  // namespace esp_adf
}  // namespace esphome

#endif
