#pragma once

#ifdef USE_ESP_IDF

#include "esphome/components/media_player/media_player.h"

#include "../adf_pipeline_controller.h"
#include "../adf_audio_sources.h"
#include "udp_mrm.h"

namespace esphome {
namespace esp_adf {

class ADFUriTrack {
  public:
    std::string uri{""};
    bool is_played{false};
};

class ADFPlaylistTrack : public ADFUriTrack {
  public:
    unsigned int order{0};
    std::string artist{""};
    std::string album{""};
    std::string title{""};
    int duration{0};
    int64_t filesize{0};
    
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
  std::string repeat() const { return media_player_repeat_mode_to_string(this->repeat_); }
  bool is_shuffle() const override { return this->shuffle_; }
  std::string artist() const { return this->artist_; }
  std::string album() const { return this->album_; }
  std::string title() const { return this->title_; }
  int duration() const { return this->duration_; }
  int position() const { return this->position_; }
  media_player::MediaPlayerTraits get_traits() override;
  
  void loop();

 protected:
  // MediaPlayer implementation
  void control(const media_player::MediaPlayerCall &call) override;

  // Pipeline implementations
  void on_pipeline_state_change(PipelineState state);
    
  void start_();
  void stop_();

  void mrm_process_recv_actions_() ;
  void mrm_set_stream_uri_(const std::string url);
  void mrm_start_();
  void mrm_stop_();
  bool mrm_listen_requested_{false};
  void mrm_listen_();
  void mrm_unlisten_();
  void mrm_turn_on_();
  void mrm_turn_off_();
  void mrm_volume_();
  void mrm_mute_();
  void mrm_unmute_();

  void mute_();
  void unmute_();
  void set_mrm_(media_player::MediaPlayerMRM mrm);
  void set_repeat_(media_player::MediaPlayerRepeatMode repeat);
  void set_shuffle_(bool shuffle);
  void set_artist_(const std::string& artist) {artist_ = artist;}
  void set_album_(const std::string& album) {album_ = album;}
  void set_title_(const std::string& title) {title_ = title;}
  void set_duration_(int duration) {duration_ = duration;}
  void set_filesize_(int64_t filesize) {filesize_ = filesize;}
  void set_position_(int position) {position_ = position;}
  void set_position_();
  void set_volume_(float volume, bool publish = true);

  bool muted_{false};
  media_player::MediaPlayerMRM mrm_{media_player::MEDIA_PLAYER_MRM_OFF};
  media_player::MediaPlayerRepeatMode repeat_{media_player::MEDIA_PLAYER_REPEAT_OFF};
  bool shuffle_{false};
  std::string artist_{""};
  std::string album_{""};
  std::string title_{""};
  int duration_{0}; // in seconds
  int64_t filesize_{0};
  int position_{0}; // in seconds
  std::string group_members_{""};
  bool play_intent_{false};
  bool turning_off_{false};
  bool force_publish_{false};

  HTTPStreamReaderAndDecoder http_and_decoder_;

  std::vector<ADFUriTrack > announcements_;
  void clean_announcements_();
  bool play_next_track_on_announcements_();

  std::vector<ADFPlaylistTrack > playlist_;
  int play_track_id_{-1};
  
  void update_playlist_order(unsigned int start_order);
  void play_next_track_on_playlist_(int track_id);
  void clean_playlist_();
  int next_playlist_track_id_();
  int previous_playlist_track_id_();
  void set_playlist_track_as_played_(int track_id);
  void playlist_add_(const std::string& new_uri, bool toBack);
  int parse_m3u_into_playlist_(const char *url, bool toBack);
  void set_playlist_track_(ADFPlaylistTrack track);

  bool mrm_leader_started_{false};
  UdpMRM udpMRM_;
};

}  // namespace esp_adf
}  // namespace esphome

#endif
