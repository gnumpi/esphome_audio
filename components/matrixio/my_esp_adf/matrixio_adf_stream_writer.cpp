#include "matrixio_adf_stream_writer.h"
#include <freertos/FreeRTOS.h>
#include <algorithm>
#include "raw_stream.h"

namespace esphome {
namespace matrixio {

#define CHUNK_SIZE 512
static const char *const TAG = "matrixio_adf_stream_writer";

static esp_err_t _matrixio_open(audio_element_handle_t self){
  //this_writer->flush_fpga_fifo_();
  //delay(10);
   esp_err_t res = audio_element_set_input_timeout(self, 2000 / portTICK_RATE_MS);
   return res;
}

static audio_element_err_t _matrixio_write(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    //return (audio_element_err_t) len;
    MatrixIOStreamWriter *this_writer = (MatrixIOStreamWriter *) audio_element_getdata(self);
    size_t bytes_written = 0;
    if (len) {
        if( this_writer->channels == 1){
          uint16_t tmp_buffer[CHUNK_SIZE];
          uint16_t* dst = tmp_buffer;
          const uint16_t* src = reinterpret_cast<const uint16_t*>(buffer);
          for (size_t c=0; c < len / sizeof(uint16_t); c++ ){
             memcpy( dst++, src,   sizeof(uint16_t) );
             memcpy( dst++, src++, sizeof(uint16_t) );
          }
          bytes_written = this_writer->write_to_fpga_fifo(  (uint8_t*) tmp_buffer,        len );
          bytes_written = this_writer->write_to_fpga_fifo( ((uint8_t*) tmp_buffer) + len, len );
        }
        else{
          bytes_written = this_writer->write_to_fpga_fifo( (uint8_t*)buffer, len );
        }
        
    }
    return (audio_element_err_t) bytes_written;
}

static audio_element_err_t _adf_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
  //vTaskDelay( pdMS_TO_TICKS( 10 ) );
  //return (audio_element_err_t) in_len;
  int r_size = audio_element_input(self, in_buffer, in_len);
  int w_size = 0;
  if (r_size == AEL_IO_TIMEOUT) {
    memset(in_buffer, 0x00, in_len);
    r_size = in_len;
  }
  if (r_size > 0) {
    w_size = audio_element_output(self, in_buffer, r_size);
    audio_element_update_byte_pos(self, w_size);
  }
  return (audio_element_err_t) w_size;
}


void MatrixIOStreamWriter::setup(){
  ESP_LOGCONFIG(TAG, "Setting up MatrixIO ADF-Stream-Writer...");
  this->set_pcm_sampling_frequency(44100);
  this->write_output_select_();
  this->unmute_();
  this->write_volume_();
}

void MatrixIOStreamWriter::dump_config(){
  esph_log_config(TAG, "Matrixio ADF-Stream-Writer:");
  uint32_t sampling_frequency = this->read_pcm_sampling_frequency();
  uint8_t volume = this->get_volume();
  esph_log_config(TAG, "  Sampling frequency: %uHz", (unsigned) (sampling_frequency));
  if (this->output_selector_ == kHeadPhone ){
    esph_log_config(TAG, "  Output: Headphone");
  }
  else if (this->output_selector_ == kSpeaker ){
    esph_log_config(TAG, "  Output: Speaker");
  }
  else {
    esph_log_config(TAG, "  Output not set");

  }
  esph_log_config(TAG, "  Volume: %d%%", volume);
}

void MatrixIOStreamWriter::loop(){
  
}

void MatrixIOStreamWriter::init_adf_elements_(){
  if( this->sdk_audio_elements_.size() > 0 )
    return;
  
  audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
  cfg.open = _matrixio_open;
  //cfg.close = NULL;
  cfg.process = _adf_process;
  cfg.write = _matrixio_write;
  //cfg.destroy = nullptr;
  cfg.task_stack = DEFAULT_ELEMENT_STACK_SIZE; //-1; //3072+512;
  cfg.task_prio = 0;
  cfg.task_core = 0;
  cfg.stack_in_ext = false;
  cfg.out_rb_size = 4 * CHUNK_SIZE;
  cfg.multi_out_rb_num = 0;
  cfg.tag = "matrixio";
  cfg.buffer_len = CHUNK_SIZE;

  this->audio_raw_stream_ = audio_element_init(&cfg);
  audio_element_setdata(this->audio_raw_stream_, this);
  
  sdk_audio_elements_.push_back( this->audio_raw_stream_ );
  sdk_element_tags_.push_back("audio_out");   
}


size_t MatrixIOStreamWriter::write_to_fpga_fifo( uint8_t* buffer, int len ) {
  //this_writer->flush_fpga_fifo_();
  //delay(10);
  if (len <= 0 ){
    return len;
  }
  const float sample_time = 1.0 / this->pcm_sampling_frequency_;
  const int sleep = int(MAX_WRITE_LENGTH * sample_time * 1000);
  uint16_t fifo_status = this->get_fpga_fifo_status_();
  if (fifo_status > FPGA_FIFO_SIZE * 3 / 4) {
    delay(sleep);
  }
  int write_length = std::min<int>(len,CHUNK_SIZE);
  this->wb_write( (uint8_t*) buffer, write_length);
  return write_length;
}


uint16_t MatrixIOStreamWriter::get_fpga_fifo_status_() {
  uint16_t write_pointer;
  uint16_t read_pointer;
  this->reg_read(0x802, &read_pointer);
  this->reg_read(0x803, &write_pointer);

  if (write_pointer > read_pointer)
    return write_pointer - read_pointer;
  else
    return FPGA_FIFO_SIZE - read_pointer + write_pointer;
}

void MatrixIOStreamWriter::flush_fpga_fifo_() {
  this->conf_write(12, 0x0001);
  this->conf_write(12, 0x0000);
}

void MatrixIOStreamWriter::write_output_select_() {
  this->conf_write(11, this->output_selector_);
}

void MatrixIOStreamWriter::set_output(OutputSelector output_selector) {
  this->output_selector_ = output_selector;
}

void MatrixIOStreamWriter::mute_(){
  this->conf_write(10, kMute );
  this->mute_status_ = kMute;
}

void MatrixIOStreamWriter::unmute_(){
  this->conf_write(10, kUnMute );
  this->mute_status_ = kUnMute;
}

void MatrixIOStreamWriter::set_sampling_frequency(int sampling_frequency){
  set_pcm_sampling_frequency(sampling_frequency);
}

void MatrixIOStreamWriter::set_number_of_channels(int channels){
  this->lock_.lock();
  this->channels = channels;
  this->lock_.unlock();
}

void MatrixIOStreamWriter::on_settings_request(AudioPipelineSettingsRequest &request){
  if( request.sampling_rate > 0 ){
    bool success = set_pcm_sampling_frequency( request.sampling_rate );
    if( !success ){
      request.failed = true;
      request.failed_by = this;
      return;
    }
  }
  if( request.number_of_channels > 0 ){
    if( request.number_of_channels < 3){
      set_number_of_channels(request.number_of_channels);
    } else {
      request.failed = true;
      request.failed_by = this;
      return;
    } 
  }
  if( request.target_volume >= 0. ){
    set_volume( request.target_volume);
  }
  if( request.mute == 0 ){
    unmute();
  } else if (request.mute == 1){
    mute();
  }

}


bool MatrixIOStreamWriter::set_pcm_sampling_frequency(uint32_t sampling_frequency){
  uint16_t pcm_constant;
  for (int i = 0;; i++) {
    if (PCM_SAMPLING_FREQUENCIES[i][0] == 0){
        ESP_LOGE(TAG, "Unsupported sampling frequency: %u", sampling_frequency);
        this->mark_failed();
        return false;
    }
    if ( sampling_frequency == PCM_SAMPLING_FREQUENCIES[i][0]) {
      this->pcm_sampling_frequency_ = PCM_SAMPLING_FREQUENCIES[i][0];
      pcm_constant = PCM_SAMPLING_FREQUENCIES[i][1];
      break;
    }
  }
  this->conf_write( 9, pcm_constant);
  return true;
}

uint32_t MatrixIOStreamWriter::read_pcm_sampling_frequency(){
  uint16_t pcm_constant;
  this->conf_read(9, &pcm_constant);
  for (int i = 0;; i++) {
    if (pcm_constant == PCM_SAMPLING_FREQUENCIES[i][1]) {
      return PCM_SAMPLING_FREQUENCIES[i][0];
    }
  }
  ESP_LOGE(TAG, "Reading sampling frequency faild. Got : %u", pcm_constant);
  return 0;
}


void MatrixIOStreamWriter::set_target_volume(int vol){
  float volume = static_cast<float>(vol) / 100.;
  this->volume_ = volume > 1. ? 1 : (volume < 0 ? 0 : volume);
}


void MatrixIOStreamWriter::set_volume(float volume){
  this->volume_ = volume > 1. ? 1 : (volume < 0 ? 0 : volume);
  this->write_volume_();
}

float MatrixIOStreamWriter::get_volume(){
  return this->volume_;
}


void MatrixIOStreamWriter::write_volume_(){
  float volume = (1. - this->volume_) * static_cast<float>(MAX_VOLUME_VALUE);
  int  volume_constant =  int(volume) < 0 ?  0 : (int(volume) > MAX_VOLUME_VALUE ? MAX_VOLUME_VALUE : int(volume));
  this->conf_write(8, (uint16_t) volume_constant);
}


void MatrixIOStreamWriter::read_volume(){
  uint16_t volume_constant;
  this->conf_read(8, &volume_constant);
  this->volume_ = 1. - static_cast<float>(volume_constant) / static_cast<float>(MAX_VOLUME_VALUE) / 100.;
}


}
}