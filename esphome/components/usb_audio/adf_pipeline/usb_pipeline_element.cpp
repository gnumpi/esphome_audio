#include "usb_pipeline_element.h"

#ifdef USE_ESP_IDF

#include "../../adf_pipeline/adf_pipeline.h"
#include "freertos/event_groups.h"

#include "../../adf_pipeline/sdk_ext.h"
namespace esphome {
using namespace esp_adf;
namespace usb_audio {

static const char *const TAG = "adf_usb_audio";
static const uint16_t CHUNK_SIZE = 1024;

static esp_err_t _usb_open(audio_element_handle_t self){
  USBStreamWriter *this_writer = (USBStreamWriter *) audio_element_getdata(self);

  usb_streaming_control(STREAM_UAC_SPK, CTRL_RESUME, NULL);
  usb_streaming_control(STREAM_UAC_SPK, CTRL_UAC_MUTE, 0);
  return ESP_OK;
}

static esp_err_t _usb_close(audio_element_handle_t self){
  esp_err_t ret = usb_streaming_control(STREAM_UAC_SPK, CTRL_SUSPEND, NULL);
  if( ret != ESP_OK ){
    esph_log_e(TAG, "Error USB streaming suspending failed." );
  }
  return ESP_OK;
}

static audio_element_err_t _usb_write(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    //USBStreamWriter *this_writer = (USBStreamWriter *) audio_element_getdata(self);
    int bytes_written = 0;
    if (len) {
        esp_err_t ret = uac_spk_streaming_write( buffer, len, ticks_to_wait);
        if (ret == ESP_OK) {
            //esph_log_d(TAG, "written: %d", len );
            bytes_written = len;
        }
        else{
          esph_log_e(TAG, "error writing to usb stream %d", ret );
        }
    }
    return (audio_element_err_t) bytes_written;
}

static audio_element_err_t _adf_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
  int r_size = audio_element_input(self, in_buffer, in_len);
  int w_size = 0;

  if (r_size > 0) {
    w_size = audio_element_output(self, in_buffer, r_size);
    audio_element_update_byte_pos(self, w_size);
  }
  return (audio_element_err_t) w_size;
}

bool USBStreamWriter::init_adf_elements_(){
  if( this->sdk_audio_elements_.size() > 0 )
    return true;

  audio_element_cfg_t cfg{};
  cfg.open = _usb_open;
  cfg.seek = nullptr;
  cfg.process = _adf_process;
  cfg.close = _usb_close;
  cfg.destroy = nullptr;
  cfg.write = _usb_write;

  cfg.buffer_len = CHUNK_SIZE;
  cfg.task_stack = 2 * DEFAULT_ELEMENT_STACK_SIZE; //-1; //3072+512;
  cfg.task_prio = 5;
  cfg.task_core = 0;
  cfg.out_rb_size = 4 * CHUNK_SIZE;
  cfg.data = nullptr;
  cfg.tag = "usbwriter";
  cfg.stack_in_ext = true;
  cfg.multi_out_rb_num = 0;
  cfg.multi_in_rb_num = 0;

  this->usb_audio_stream_ = audio_element_init(&cfg);
  audio_element_setdata(this->usb_audio_stream_, this);

  sdk_audio_elements_.push_back( this->usb_audio_stream_ );
  sdk_element_tags_.push_back("usb_out");

  this->setup_usb();
  this->start_streaming();
  ESP_ERROR_CHECK(usb_streaming_connect_wait(portMAX_DELAY));
  usb_streaming_control(STREAM_UAC_SPK, CTRL_SUSPEND, NULL);
  esp_err_t ret = uac_frame_size_reset(STREAM_UAC_SPK, 2, 16, 48000);
  return true;
}

bool USBStreamWriter::preparing_step(){
  audio_element_state_t curr_state = audio_element_get_state(this->usb_audio_stream_);
  //esph_log_d(TAG, "USB status: %d", curr_state);
  if(curr_state == AEL_STATE_RUNNING){
    audio_element_pause(this->usb_audio_stream_);
  } else if( curr_state != AEL_STATE_PAUSED){
    if( audio_element_run(this->usb_audio_stream_) != ESP_OK )
    {
      esph_log_e(TAG, "Starting USB stream element failed");
    }
    if (audio_element_pause(this->usb_audio_stream_) ){
      esph_log_e(TAG, "Pausing USB stream element failed");
    }
  }
  return true;
}

bool USBStreamWriter::is_ready(){
  return true;
  if( !this->usb_stream_started_ )
  {
    //this->start_streaming();
    this->usb_stream_started_ = true;
  }
  return true;//usb_streaming_connect_wait(10 / portTICK_RATE_MS) == ESP_OK;
}


void USBStreamWriter::reset_(){
  this->usb_stream_started_ = false;
}

void USBStreamWriter::clear_adf_elements_(){
  this->usb_stream_started_ = false;
  this->sdk_audio_elements_.clear();
  this->sdk_element_tags_.clear();
}


void USBStreamWriter::on_settings_request(AudioPipelineSettingsRequest &request) {
  if ( !this->usb_audio_stream_ ){
    return;
  }

  if (request.sampling_rate == -1 ){
    return;
  }

  if (request.final_sampling_rate == -1) {
    request.final_sampling_rate = this->sample_rate_;
    request.final_bit_depth = this->bits_per_sample_;
    request.final_number_of_channels = this->channels_;
  } else if (
       request.final_sampling_rate != this->sample_rate_
    || request.final_bit_depth != this->bits_per_sample_
    || request.final_number_of_channels != this->channels_
  )
  {
    request.failed = true;
    request.failed_by = this;
  }

}

}
}
#endif
