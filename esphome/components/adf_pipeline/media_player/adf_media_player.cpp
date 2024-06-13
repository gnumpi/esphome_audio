#include "adf_media_player.h"
#include "esphome/core/log.h"
#include "esphome/components/api/homeassistant_service.h"
#include "esphome/components/api/custom_api_device.h"

#include <algorithm>
#include <random>

#ifdef USE_ESP_IDF
#include <esp_http_client.h>

namespace esphome {
namespace esp_adf {

static const char *const TAG = "adf_media_player";

void ADFMediaPlayer::setup() {
  state = media_player::MEDIA_PLAYER_STATE_OFF;
  set_volume_(.01);
}

void ADFMediaPlayer::dump_config() {
  esph_log_config(TAG, "ESP-ADF-MediaPlayer:");
  int components = pipeline.get_number_of_elements();
  esph_log_config(TAG, "  Number of ASPComponents: %d", components);
}

void ADFMediaPlayer::publish_state() {
  esph_log_d(TAG, "MP State = %s, MP Prior State = %s", media_player_state_to_string(state), media_player_state_to_string(prior_state));
  if (state != prior_state || force_publish_) {
  esph_log_d(TAG, "Publish State");
    this->state_callback_.call();
    this->prior_state = this->state;
    this->force_publish_ = false;
  }
}

media_player::MediaPlayerTraits ADFMediaPlayer::get_traits() {
  auto traits = media_player::MediaPlayerTraits();
  traits.set_supports_pause( true );
  traits.set_supports_next_previous_track( true );
  traits.set_supports_turn_off_on( true );
  traits.set_supports_grouping( true );
  return traits;
}

void ADFMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  
    esph_log_d(TAG, "Got control call in state %d", state);
    

    media_player::MediaPlayerMRM mrm = media_player::MEDIA_PLAYER_MRM_OFF;
    if (group_members_.length() > 0) {
      mrm = media_player::MEDIA_PLAYER_MRM_LEADER;
    }

  //Media File is sent (no command)
  if (call.get_media_url().has_value())
  {
    if (call.get_media_url().value() == "listen") {
      mrm = media_player::MEDIA_PLAYER_MRM_FOLLOWER;
      mrm_listen_();
    }
    else if (call.get_media_url().value() == "unlisten") {
      mrm = media_player::MEDIA_PLAYER_MRM_FOLLOWER;
      mrm_unlisten_();
    }
    else {
      //TBD announcing integration into pipeline
      bool announcing = false;
      if (call.get_announcement().has_value()) {
        announcing = call.get_announcement().value();
      }

      if (call.get_mrm().has_value()) {
        mrm = call.get_mrm().value();
      }
      
      media_player::MediaPlayerEnqueue enqueue = media_player::MEDIA_PLAYER_ENQUEUE_PLAY;
      if (call.get_enqueue().has_value()) {
        enqueue = call.get_enqueue().value();
      }
      if (enqueue == media_player::MEDIA_PLAYER_ENQUEUE_REPLACE || enqueue == media_player::MEDIA_PLAYER_ENQUEUE_PLAY) {
        this->play_track_id_ = -1;
        if (enqueue == media_player::MEDIA_PLAYER_ENQUEUE_REPLACE) {
          clean_playlist_track_();
        }
        playlist_add_(call.get_media_url().value(), true);
        set_playlist_track_(playlist_[0]);
        this->play_intent_ = true;
        if (state == media_player::MEDIA_PLAYER_STATE_PLAYING || state == media_player::MEDIA_PLAYER_STATE_PAUSED) {
          this->play_track_id_ = 0;
          stop();
          return;
        } else {
          if (state == media_player::MEDIA_PLAYER_STATE_OFF) {
            state = media_player::MEDIA_PLAYER_STATE_ON;
            publish_state();
          }
          start();
        }
      }
      else if (enqueue == media_player::MEDIA_PLAYER_ENQUEUE_ADD) {
        playlist_add_(call.get_media_url().value(), true);
      }
      else if (enqueue == media_player::MEDIA_PLAYER_ENQUEUE_NEXT) {
        playlist_add_(call.get_media_url().value(), false);
      }
    }
  }
  // Volume value is sent (no command)
  if (call.get_volume().has_value()) {
    set_volume_(call.get_volume().value());
    unmute_();
  }
  set_mrm_(mrm);
  //Command
  if (call.get_command().has_value()) {
    switch (call.get_command().value()) {
      case media_player::MEDIA_PLAYER_COMMAND_PLAY:
        this->play_intent_ = true;
        this->play_track_id_ = -1;
        if (state == media_player::MEDIA_PLAYER_STATE_PLAYING) {
          stop();
        }
        if (state == media_player::MEDIA_PLAYER_STATE_PAUSED) {
          pipeline.resume();
        }
        if (state == media_player::MEDIA_PLAYER_STATE_OFF 
        || state == media_player::MEDIA_PLAYER_STATE_ON 
        || state == media_player::MEDIA_PLAYER_STATE_NONE
        || state == media_player::MEDIA_PLAYER_STATE_IDLE) {
        
          int id = next_playlist_track_id_();
          if (id > -1) {
            set_playlist_track_(playlist_[id]);
          }
          if (state == media_player::MEDIA_PLAYER_STATE_OFF) {
            state = media_player::MEDIA_PLAYER_STATE_ON;
            publish_state();
          }
          start();
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_PAUSE:
        if (state == media_player::MEDIA_PLAYER_STATE_PLAYING) {
          this->play_track_id_ = next_playlist_track_id_();
          play_intent_ = false;
        }
        stop();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_STOP:
        play_intent_ = false;
        clean_playlist_track_();
        stop();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_MUTE:
        this->mute_();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_UNMUTE:
        this->unmute_();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_UP: {
        float new_volume = this->volume + 0.1f;
        if (new_volume > 1.0f)
          new_volume = 1.0f;
        set_volume_(new_volume);
        unmute_();
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_DOWN: {
        float new_volume = this->volume - 0.1f;
        if (new_volume < 0.0f)
          new_volume = 0.0f;
        set_volume_(new_volume);
        unmute_();
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_NEXT_TRACK: {
        if ( this->playlist_.size() > 0 ) {
          this->play_intent_ = true;
          this->play_track_id_ = -1;
          stop();
        }
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_PREVIOUS_TRACK: {
        if ( this->playlist_.size() > 0 ) {
          this->play_intent_ = true;
          this->play_track_id_ = previous_playlist_track_id_();
          stop();
        }
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_TOGGLE: {
        if (state == media_player::MEDIA_PLAYER_STATE_OFF) {
          state = media_player::MEDIA_PLAYER_STATE_ON;
          publish_state();
          mrm_turn_on_();
        }
        else {
          if (state == media_player::MEDIA_PLAYER_STATE_PLAYING 
          || state == media_player::MEDIA_PLAYER_STATE_PAUSED 
          || state == media_player::MEDIA_PLAYER_STATE_ANNOUNCING ) {
            turning_off_ = true;
            this->play_intent_ = false;
            stop();
          }
          else {
            state = media_player::MEDIA_PLAYER_STATE_OFF;
            publish_state();
          }
          mrm_turn_off_();
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_TURN_ON: {
        if (state == media_player::MEDIA_PLAYER_STATE_OFF) {
            state = media_player::MEDIA_PLAYER_STATE_ON;
            publish_state();
        }
        mrm_turn_on_();
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_TURN_OFF: {
        if (state != media_player::MEDIA_PLAYER_STATE_OFF) {
          if (state == media_player::MEDIA_PLAYER_STATE_PLAYING 
          || state == media_player::MEDIA_PLAYER_STATE_PAUSED 
          || state == media_player::MEDIA_PLAYER_STATE_ANNOUNCING ) {
            turning_off_ = true;
            this->play_intent_ = false;
            stop();
          }
          else {
            state = media_player::MEDIA_PLAYER_STATE_OFF;
            publish_state();
          }
        }
        mrm_turn_off_();
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_CLEAR_PLAYLIST: {
        clean_playlist_track_();
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_SHUFFLE: {
        set_shuffle_(true);
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_UNSHUFFLE: {
        set_shuffle_(false);
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_REPEAT_OFF: {
        set_repeat_(media_player::MEDIA_PLAYER_REPEAT_OFF);
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_REPEAT_ONE: {
        set_repeat_(media_player::MEDIA_PLAYER_REPEAT_ONE);
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_REPEAT_ALL: {
        set_repeat_(media_player::MEDIA_PLAYER_REPEAT_ALL);
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_JOIN: {
        if (call.get_group_members().has_value()) {
          group_members_ = call.get_group_members().value();
        }
        break;
      }
      case media_player::MEDIA_PLAYER_COMMAND_UNJOIN: {
        group_members_ = "";
        mrm_unlisten_();
        break;
      }
      default:
        break;
      }
    }
  }
}

void ADFMediaPlayer::on_pipeline_state_change(PipelineState state) {
  esph_log_i(TAG, "got new pipeline state: %d", state);
  switch (state) {
    case PipelineState::PREPARING:
    case PipelineState::STARTING:
    case PipelineState::RESUMING:
    case PipelineState::RUNNING:
      this->set_volume_( this->volume, false);
      this->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
      publish_state();
      break;
    case PipelineState::STOPPING:
      set_artist_("");
      set_album_("");
      set_title_("");
      this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
      publish_state();
      break;
    case PipelineState::STOPPED:
      this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
      publish_state();
      if (this->play_intent_ && !pipeline.is_destroy_on_stop()) {
        play_next_track_on_playlist_(this->play_track_id_);
        this->play_track_id_ = -1;
      }
      if (this->play_intent_ && !pipeline.is_destroy_on_stop()) {
        start();
      }
      if (this->turning_off_ && !pipeline.is_destroy_on_stop()) {
        this->state = media_player::MEDIA_PLAYER_STATE_OFF;
        publish_state();
        turning_off_ = false;
      }
      if (!this->play_intent_ && !pipeline.is_destroy_on_stop()){
        pipeline.reset(true);
      }
      break;
    case PipelineState::DESTROYING:
      this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
      publish_state();
      break;
    case PipelineState::UNINITIALIZED:
      this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
      publish_state();
      if (this->play_intent_ && pipeline.is_destroy_on_stop()) {
        play_next_track_on_playlist_(this->play_track_id_);
        this->play_track_id_ = -1;
      }
      if (this->play_intent_ && pipeline.is_destroy_on_stop()) {
        start();
      }
      if (this->turning_off_ && pipeline.is_destroy_on_stop()) {
        this->state = media_player::MEDIA_PLAYER_STATE_OFF;
        publish_state();
        turning_off_ = false;
      }
      break;
    case PipelineState::PAUSING:
    case PipelineState::PAUSED:
      this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
      publish_state();
      break;
    default:
      break;
  }
}

void ADFMediaPlayer::start() 
{
  mrm_listen_();
  pipeline.start();
  mrm_start_();
}

void ADFMediaPlayer::stop()
{
  mrm_unlisten_();
  if (turning_off_) {
    mrm_turn_off_();
  }
  mrm_stop_();
  pipeline.stop();
}

void ADFMediaPlayer::set_volume_(float volume, bool publish) {
  AudioPipelineSettingsRequest request;
  request.target_volume = volume;
  if (pipeline.request_settings(request)) {
    this->volume = volume;
    if (publish) {
      force_publish_ = true;
      publish_state();
    }
    mrm_volume_();
  }
}

void ADFMediaPlayer::mute_() {
  AudioPipelineSettingsRequest request;
  request.mute = 1;
  if (pipeline.request_settings(request)) {
    muted_ = true;
    force_publish_ = true;
    publish_state();
    mrm_mute_();
  }
}

void ADFMediaPlayer::unmute_() {
  AudioPipelineSettingsRequest request;
  request.target_volume = volume;
  request.mute = 0;
  if (pipeline.request_settings(request)) {
    muted_ = false;
    force_publish_ = true;
    publish_state();
    mrm_unmute_();
  }
}

void ADFMediaPlayer::mrm_listen_() {
  if (!mrm_listen_requested_) {
    esph_log_d(TAG, "mrm listen");
    if (mrm_ == media_player::MEDIA_PLAYER_MRM_LEADER || mrm_ == media_player::MEDIA_PLAYER_MRM_FOLLOWER) {
      //TBD - mrm
    }
    if (group_members_.length() > 0 && mrm_ == media_player::MEDIA_PLAYER_MRM_LEADER) {
      std::string group_members = group_members_ + ",";
      char *token = strtok(const_cast<char*>(group_members.c_str()), ",");
      esphome::api::HomeassistantServiceResponse resp;
      resp.service = "media_player.media_play";
      
      //TBD - this will require a change in core esphome media-player
      esphome::api::HomeassistantServiceMap kv2;
      kv2.key = "media_content_id";
      kv2.value = "listen";

      while (token != nullptr)
      {
        esph_log_d(TAG, "entity: %s", token);
        esphome::api::HomeassistantServiceMap kv1;
        kv1.key = "entity_id";
        kv1.value = token;
        resp.data.push_back(kv1);
        resp.data.push_back(kv2);
        esphome::api::global_api_server->send_homeassistant_service_call(resp);
        token = strtok(nullptr, ",");
      }
    }
    mrm_listen_requested_ = true;
  }
}

void ADFMediaPlayer::mrm_unlisten_() {
  
  if (!mrm_listen_requested_) {
    esph_log_d(TAG, "mrm unlisten");
    if (mrm_ == media_player::MEDIA_PLAYER_MRM_LEADER || mrm_ == media_player::MEDIA_PLAYER_MRM_FOLLOWER) {
      //TBD - mrm
    }
    if (group_members_.length() > 0 && mrm_ == media_player::MEDIA_PLAYER_MRM_LEADER) {
      std::string group_members = group_members_ + ",";
      char *token = strtok(const_cast<char*>(group_members.c_str()), ",");
      esphome::api::HomeassistantServiceResponse resp;
      resp.service = "media_player.media_play";
      
      //TBD - this will require a change in core esphome media-player
      esphome::api::HomeassistantServiceMap kv2;
      kv2.key = "media_content_id";
      kv2.value = "unlisten";

      while (token != nullptr)
      {
        esph_log_d(TAG, "entity: %s", token);
        esphome::api::HomeassistantServiceMap kv1;
        kv1.key = "entity_id";
        kv1.value = token;
        resp.data.push_back(kv1);
        resp.data.push_back(kv2);
        esphome::api::global_api_server->send_homeassistant_service_call(resp);
        token = strtok(nullptr, ",");
      }
    }
    mrm_listen_requested_ = true;
  }
}

void ADFMediaPlayer::mrm_turn_on_() {
  if (group_members_.length() > 0 && mrm_ == media_player::MEDIA_PLAYER_MRM_LEADER) {
    esph_log_d(TAG, "mrm turn_on");
    std::string group_members = group_members_ + ",";
    char *token = strtok(const_cast<char*>(group_members.c_str()), ",");
    esphome::api::HomeassistantServiceResponse resp;
    resp.service = "media_player.turn_on";
    while (token != nullptr)
    {
      esph_log_d(TAG, "entity: %s", token);
      esphome::api::HomeassistantServiceMap kv1;
      kv1.key = "entity_id";
      kv1.value = token;
      resp.data.push_back(kv1);
      esphome::api::global_api_server->send_homeassistant_service_call(resp);
      token = strtok(nullptr, ",");
    }
  }
}

void ADFMediaPlayer::mrm_turn_off_() {
  if (group_members_.length() > 0 && mrm_ == media_player::MEDIA_PLAYER_MRM_LEADER) {
    esph_log_d(TAG, "mrm turn_off");
    std::string group_members = group_members_ + ",";
    char *token = strtok(const_cast<char*>(group_members.c_str()), ",");
    esphome::api::HomeassistantServiceResponse resp;
    resp.service = "media_player.turn_off";
    while (token != nullptr)
    {
      esph_log_d(TAG, "entity: %s", token);
      esphome::api::HomeassistantServiceMap kv1;
      kv1.key = "entity_id";
      kv1.value = token;
      resp.data.push_back(kv1);
      esphome::api::global_api_server->send_homeassistant_service_call(resp);
      token = strtok(nullptr, ",");
    }
  }
}

void ADFMediaPlayer::mrm_volume_() {
  if (group_members_.length() > 0 && mrm_ == media_player::MEDIA_PLAYER_MRM_LEADER) {
    esph_log_d(TAG, "mrm volume");
    std::string group_members = group_members_ + ",";
    char *token = strtok(const_cast<char*>(group_members.c_str()), ",");
    esphome::api::HomeassistantServiceResponse resp;
    resp.service = "media_player.volume_set";
    esphome::api::HomeassistantServiceMap kv2;
    kv2.key = "volume_level";
    kv2.value = to_string(volume);

    while (token != nullptr)
    {
      esph_log_d(TAG, "entity: %s", token);
      esphome::api::HomeassistantServiceMap kv1;
      kv1.key = "entity_id";
      kv1.value = token;
      resp.data.push_back(kv1);
      resp.data.push_back(kv2);

      esphome::api::global_api_server->send_homeassistant_service_call(resp);
      token = strtok(nullptr, ",");
    }
  }
}

void ADFMediaPlayer::mrm_mute_() {
  if (group_members_.length() > 0 && mrm_ == media_player::MEDIA_PLAYER_MRM_LEADER) {
    esph_log_d(TAG, "mrm mute");
    std::string group_members = group_members_ + ",";
    char *token = strtok(const_cast<char*>(group_members.c_str()), ",");
    esphome::api::HomeassistantServiceResponse resp;
    resp.service = "media_player.volume_mute";
    esphome::api::HomeassistantServiceMap kv2;
    kv2.key = "is_volume_muted";
    kv2.value = "true";

    while (token != nullptr)
    {
      esph_log_d(TAG, "entity: %s", token);
      esphome::api::HomeassistantServiceMap kv1;
      kv1.key = "entity_id";
      kv1.value = token;
      resp.data.push_back(kv1);
      resp.data.push_back(kv2);

      esphome::api::global_api_server->send_homeassistant_service_call(resp);
      token = strtok(nullptr, ",");
    }
  }
}

void ADFMediaPlayer::mrm_unmute_() {
  if (group_members_.length() > 0 && mrm_ == media_player::MEDIA_PLAYER_MRM_LEADER) {
    esph_log_d(TAG, "mrm unmute");
    std::string group_members = group_members_ + ",";
    char *token = strtok(const_cast<char*>(group_members.c_str()), ",");
    esphome::api::HomeassistantServiceResponse resp;
    resp.service = "media_player.volume_mute";
    esphome::api::HomeassistantServiceMap kv2;
    kv2.key = "is_volume_muted";
    kv2.value = "false";

    while (token != nullptr)
    {
      esph_log_d(TAG, "entity: %s", token);
      esphome::api::HomeassistantServiceMap kv1;
      kv1.key = "entity_id";
      kv1.value = token;
      resp.data.push_back(kv1);
      resp.data.push_back(kv2);

      esphome::api::global_api_server->send_homeassistant_service_call(resp);
      token = strtok(nullptr, ",");
    }
  }
}

void ADFMediaPlayer::mrm_start_() 
{
  if (!mrm_leader_started_ && group_members_.length() > 0 && mrm_ == media_player::MEDIA_PLAYER_MRM_LEADER) {
      //TBD - mrm
      mrm_leader_started_ = true;
  }
}

void ADFMediaPlayer::mrm_stop_() 
{
  if (mrm_leader_started_ && group_members_.length() > 0 && mrm_ == media_player::MEDIA_PLAYER_MRM_LEADER) {
      //TBD - mrm
      mrm_leader_started_ = true;
  }
}

void ADFMediaPlayer::set_mrm_(media_player::MediaPlayerMRM mrm) {
  this->mrm_ = mrm;
}

void ADFMediaPlayer::set_repeat_(media_player::MediaPlayerRepeatMode repeat) {
  this->repeat_ = repeat;
  force_publish_ = true;
  publish_state();
}

void ADFMediaPlayer::set_shuffle_(bool shuffle) {
  unsigned int vid = this->playlist_.size();
  
  if (vid > 0) {
    if (shuffle) {
      auto rng = std::default_random_engine {};
      std::shuffle(std::begin(playlist_), std::end(playlist_), rng);
    }
    else {
      sort(playlist_.begin(), playlist_.end());
    }
    for(unsigned int i = 0; i < vid; i++)
    {
      //esph_log_d(TAG, "Playlist: %s", playlist_[i].uri.c_str());
      this->playlist_[i].is_played = false;
    }
    
    this->shuffle_ = shuffle;
    force_publish_ = true;
    publish_state();
    this->play_intent_ = true;
    this->play_track_id_ = 0;
    stop();
  }
}

void ADFMediaPlayer::playlist_add_(const std::string& uri, bool toBack) {
  
  esph_log_d(TAG, "playlist_add_: %s", uri.c_str());

  unsigned int vid = this->playlist_.size();

  if (uri.find("m3u") != std::string::npos) {
    int pl = parse_m3u_into_playlist_(uri.c_str(), toBack);
  }
  else {
    if (!toBack) {
      update_playlist_order(1);
      vid = 0;
    }
    ADFPlaylistTrack track;
    track.uri = uri;
    track.order = 0;
    playlist_.push_back(track);
  }
  
  esph_log_d(TAG, "Number of playlist tracks = %d", playlist_.size());
}

void ADFMediaPlayer::set_playlist_track_(ADFPlaylistTrack track) {
  esph_log_d(TAG, "%s %s %s", track.artist.c_str(), track.album.c_str(), track.title.c_str());
  set_artist_(track.artist);
  set_album_(track.album);
  if (track.title == "") {
    set_title_(track.uri);
  }
  else {
    set_title_(track.title);
  }
  http_and_decoder_.set_stream_uri(track.uri);
}

void ADFMediaPlayer::update_playlist_order(unsigned int start_order) {
  unsigned int vid = this->playlist_.size();
  if (vid > 0) {
    sort(playlist_.begin(), playlist_.end());
    for(unsigned int i = 0; i < vid; i++)
    {
      this->playlist_[i].order = i + start_order;
    }
  }
}

void ADFMediaPlayer::play_next_track_on_playlist_(int track_id) {

  unsigned int vid = this->playlist_.size();
  if (playlist_.size() > 0) {
    if (repeat_ != media_player::MEDIA_PLAYER_REPEAT_ONE) {
      set_playlist_track_as_played_(track_id);
    }
    int id = next_playlist_track_id_();
    if (id > -1) {
      set_playlist_track_(playlist_[id]);
    }
    else {
      if (repeat_ == media_player::MEDIA_PLAYER_REPEAT_ALL) {
        for(unsigned int i = 0; i < vid; i++)
        {
          this->playlist_[i].is_played = false;
        }
        set_playlist_track_(playlist_[0]);
      }
      else {
        clean_playlist_track_();
        this->play_intent_ = false;
      }
    }
  }
}

void ADFMediaPlayer::clean_playlist_track_()
{
  if ( this->playlist_.size() > 0 ) {
    unsigned int vid = this->playlist_.size();
    /*
    for(unsigned int i = 0; i < vid; i++)
    {
      delete this->&playlist_[i];
    }
    */
    this->playlist_.clear();
    play_track_id_ = -1;
  }
}

int ADFMediaPlayer::next_playlist_track_id_()
{
  unsigned int vid = this->playlist_.size();
  if ( vid > 0 ) {
    for(unsigned int i = 0; i < vid; i++)
    {
      bool ip = this->playlist_[i].is_played;
      if (!ip) {
        return i;
      }
    }
  }
  return -1;
}

int ADFMediaPlayer::previous_playlist_track_id_()
{
  unsigned int vid = this->playlist_.size();
  if ( vid > 0 ) {
    for(unsigned int i = 0; i < vid; i++)
    {
      bool ip = this->playlist_[i].is_played;
      if (!ip) {
        int j = i-1;
        if (j < 0) j = 0;
        this->playlist_[j].is_played = false;
        return j;
      }
    }
  }
  return -1;
}

void ADFMediaPlayer::set_playlist_track_as_played_(int track_id)
{
  if ( this->playlist_.size() > 0 ) {
      unsigned int vid = this->playlist_.size();
    if (track_id < 0) {
      for(unsigned int i = 0; i < vid; i++)
      {
        bool ip = this->playlist_[i].is_played;
        if (!ip) {
          this->playlist_[i].is_played = true;
          break;
        }
      }
    }
    else {
      for(unsigned int i = 0; i < vid; i++)
      {
        if (i < track_id) {
          this->playlist_[i].is_played = true;
        }
        else {
          this->playlist_[i].is_played = false;
        }
      }
    }
  }
}

int ADFMediaPlayer::parse_m3u_into_playlist_(const char *url, bool toBack)
{
  
    unsigned int vid = this->playlist_.size();
    esp_http_client_config_t config = {
        .url = url
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    char *response;
    esp_err_t err = esp_http_client_open(client,0);
    int rc = 0;
    if (err == ESP_OK) {
      //int cl = esp_http_client_get_content_length(client);
      int cl =  esp_http_client_fetch_headers(client);
      //esph_log_v(TAG, "HTTP Status = %d, content_length = %d", esp_http_client_get_status_code(client), cl);
      response = (char *)malloc(cl + 1);
      if (response == NULL) {
        esph_log_e(TAG, "Cannot malloc http receive buffer");
        rc = -1;
      }
      else {
        int rl = esp_http_client_read(client, response, cl); 
        if (rl < 0) {
          esph_log_e(TAG, "HTTP request failed: %s, ", esp_err_to_name(err));
          free(response);
          rc = -1;
        }
        else {
          response[cl] = '\0';
          //esph_log_v(TAG, "Response: %s", response);
          size_t start = 0;
          size_t end;
          char *ptr;
          char *buffer = response;
          bool keeplooping = true;
          std::string artist = "";
          std::string album = "";
          std::string title = "";
          if (toBack) {
            update_playlist_order(1000);
            vid = 0;
          }
          while (keeplooping) {
            ptr = strchr(buffer , '\n' );
            if (ptr != NULL)
            {
              end = ptr - response;
            }
            else {
              end = cl;
              keeplooping = false;
            }
            size_t lngth = (end - start);
            //Finding a carriage return, assume its a Windows file and this line ends with "\r\n"
            if (strchr(buffer,'\r') != NULL) {
              lngth--;
            }
            if (lngth > 0) {
              char *cLine;
              cLine = (char *)malloc(lngth + 1);
              if (cLine == NULL) {
                esph_log_e(TAG, "Cannot malloc cLine");
                free(response);
                rc = -1;
                break;
              }
              sprintf (cLine,"%.*s", lngth, buffer);
              cLine[lngth] = '\0';
              esph_log_d(TAG, "%s", cLine);
              if (strstr(cLine,"#EXTART:") != NULL) {
                artist = cLine + 8;
              }
              else if (strstr(cLine,"#EXTALB:") != NULL) {
                if (strchr(cLine,'-') != NULL) {
                  album = strchr(cLine,'-') + 1;
                }
                else {
                  album = cLine + 8;
                }
              }
              else if (strstr(cLine,"#EXTINF:") != NULL) {
                title = strchr(cLine,',') + 1;
              }
              else if (strchr(cLine,'#') == NULL) {
                ADFPlaylistTrack track;
                track.uri = cLine;
                track.order = vid;
                track.artist = artist;
                track.album = album;
                track.title = title;
                playlist_.push_back(track);
                vid++;
              }
              free(cLine);
            }
            start = end + 1;
            buffer = ptr + 1;
          }
          free(response);
        }
      }
    } else {
        esph_log_e(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        rc = -1;
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    
    if (toBack) {
        update_playlist_order(0);
    }
    if (shuffle_) {
      auto rng = std::default_random_engine {};
      std::shuffle(std::begin(playlist_), std::end(playlist_), rng);
      unsigned int vid = this->playlist_.size();
      for(unsigned int i = 0; i < vid; i++)
      {
        esph_log_v(TAG, "Playlist: %s", playlist_[i].uri.c_str());
        this->playlist_[i].is_played = false;
      }
    }
    return rc;
}

}  // namespace esp_adf
}  // namespace esphome
#endif
