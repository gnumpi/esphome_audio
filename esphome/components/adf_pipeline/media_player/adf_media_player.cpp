#include "adf_media_player.h"
#include "esphome/core/log.h"

#include <esp_http_client.h>

#ifdef USE_ESP_IDF

namespace esphome {
namespace esp_adf {

static const char *const TAG = "adf_media_player";

void ADFMediaPlayer::setup() {
  state = media_player::MEDIA_PLAYER_STATE_IDLE;
}

void ADFMediaPlayer::dump_config() {
  esph_log_config(TAG, "ESP-ADF-MediaPlayer:");
  int components = pipeline.get_number_of_elements();
  esph_log_config(TAG, "  Number of ASPComponents: %d", components);
}

void ADFMediaPlayer::set_stream_uri(const std::string& new_uri) {
  std::string uri = new_uri;
  playlist_found_ = false;
  clean_playlist_track_();
  if (new_uri.find("m3u") != std::string::npos) {
    esph_log_d(TAG, "Playlist Found" );
    playlist_found_ = true;

    //tbd - get file contents and parse
    int pl = parse_m3u_into_playlist_(uri.c_str());
    if (pl < 0) {
      return;
    }
    esph_log_d(TAG, "Number of playlist tracks = %d", playlist_.size());
    int id = next_playlist_track_id_();

    uri = playlist_[id].uri;
  }
  
  ADFEncoding decoder_type = http_and_decoder_.decoder_type;

  if (uri.find(".aac") != std::string::npos) {
    http_and_decoder_.decoder_type = ADFEncoding::AAC;
  }
  else if (uri.find(".amr") != std::string::npos) {
    http_and_decoder_.decoder_type = ADFEncoding::AMR;
  }
  else if (uri.find(".flac") != std::string::npos) {
    http_and_decoder_.decoder_type = ADFEncoding::FLAC;
  }
  else if (uri.find(".mp3") != std::string::npos) {
    http_and_decoder_.decoder_type = ADFEncoding::MP3;
  }
  else if (uri.find(".ogg") != std::string::npos) {
    http_and_decoder_.decoder_type = ADFEncoding::OGG;
  }
  else if (uri.find(".opus") != std::string::npos) {
    http_and_decoder_.decoder_type = ADFEncoding::OPUS;
  }
  else {
    http_and_decoder_.decoder_type = ADFEncoding::WAV;
  }
  http_and_decoder_.set_stream_uri(uri);
}

void ADFMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  if (call.get_media_url().has_value()) {
    set_stream_uri( call.get_media_url().value()) ;

    esph_log_d(TAG, "Got control call in state %d", state );
    if (state == media_player::MEDIA_PLAYER_STATE_PLAYING || state == media_player::MEDIA_PLAYER_STATE_PAUSED) {
      this->play_intent_ = true;
      pipeline.stop();
      return;
    } else {
      pipeline.start();
    }
  }

  if (call.get_volume().has_value()) {
    set_volume_(call.get_volume().value());
    unmute_();
  }

  if (call.get_command().has_value()) {
    switch (call.get_command().value()) {
      case media_player::MEDIA_PLAYER_COMMAND_PLAY:
        if (pipeline.getState() == PipelineState::STOPPED || pipeline.getState() == PipelineState::UNINITIALIZED) {
          pipeline.start();
        } else if (pipeline.getState() == PipelineState::PAUSED) {
          pipeline.resume();
        }
        else if (state == media_player::MEDIA_PLAYER_STATE_PLAYING || state == media_player::MEDIA_PLAYER_STATE_PAUSED){
          this->play_intent_ = true;
          pipeline.stop();
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_PAUSE:
        if (pipeline.getState() == PipelineState::RUNNING) {
          pipeline.pause();
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_STOP:
        clean_playlist_track_();
        pipeline.stop();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_MUTE:
        this->mute_();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_UNMUTE:
        this->unmute_();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_TOGGLE:
        if (pipeline.getState() == PipelineState::STOPPED || pipeline.getState() == PipelineState::PAUSED) {
          state = media_player::MEDIA_PLAYER_STATE_PLAYING;
        } else {
          state = media_player::MEDIA_PLAYER_STATE_PAUSED;
        }
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
    }
  }
}

// pausing is only supported if destroy_pipeline_on_stop is disabled
media_player::MediaPlayerTraits ADFMediaPlayer::get_traits() {
  auto traits = media_player::MediaPlayerTraits();
  traits.set_supports_pause( false );
  return traits;
};

void ADFMediaPlayer::mute_() {
  AudioPipelineSettingsRequest request;
  request.mute = 1;
  if (pipeline.request_settings(request)) {
    muted_ = true;
    publish_state();
  }
}

void ADFMediaPlayer::unmute_() {
  AudioPipelineSettingsRequest request;
  request.mute = 0;
  if (pipeline.request_settings(request)) {
    muted_ = false;
    publish_state();
  }
}

void ADFMediaPlayer::set_volume_(float volume, bool publish) {
  AudioPipelineSettingsRequest request;
  request.target_volume = volume;
  if (pipeline.request_settings(request)) {
    this->volume = volume;
    if (publish)
      publish_state();
  }
}

void ADFMediaPlayer::on_pipeline_state_change(PipelineState state) {
  esph_log_i(TAG, "got new pipeline state: %d", (int) state);
  switch (state) {
    case PipelineState::UNINITIALIZED:
      if (playlist_found_ && pipeline.is_destroy_on_stop()) {
        play_next_song_on_playlist_();
      }
    case PipelineState::STOPPED:
      if (playlist_found_ && !pipeline.is_destroy_on_stop()) {
        play_next_song_on_playlist_();
      }
      this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
      publish_state();
      if( this->play_intent_ )
      {
        pipeline.start();
        this->play_intent_ = false;
      }
      break;
    case PipelineState::PAUSED:
      this->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
      publish_state();
      break;
    case PipelineState::PREPARING:
    case PipelineState::STARTING:
    case PipelineState::RUNNING:
      this->set_volume_( this->volume, false);
      this->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
      publish_state();
      break;
    default:
      break;
  }
}

void ADFMediaPlayer::play_next_song_on_playlist_() {
  if (playlist_found_) {
    set_playlist_track_as_played_();
    int id = next_playlist_track_id_();
    if (id > -1) {
      std::string uri = playlist_[id].uri;
      http_and_decoder_.decoder_type = ADFEncoding::MP3;
      if (uri.find(".flac") != std::string::npos) {
        http_and_decoder_.decoder_type = ADFEncoding::FLAC;
      }
      http_and_decoder_.set_stream_uri(uri);
      this->play_intent_ = true;
    }
    else {
      clean_playlist_track_();
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

void ADFMediaPlayer::set_playlist_track_as_played_()
{
  if ( this->playlist_.size() > 0 ) {
    unsigned int vid = this->playlist_.size();
    for(unsigned int i = 0; i < vid; i++)
    {
      bool ip = this->playlist_[i].is_played;
      if (!ip) {
        this->playlist_[i].is_played = true;
        break;
      }
    }
  }
}

int ADFMediaPlayer::parse_m3u_into_playlist_(const char *url)
{
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
              if (strchr(cLine,'#') == NULL) {
                ADFPlaylistTrack track;
                track.uri = cLine;
                playlist_.push_back(track);
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
    return rc;
}

}  // namespace esp_adf
}  // namespace esphome
#endif
