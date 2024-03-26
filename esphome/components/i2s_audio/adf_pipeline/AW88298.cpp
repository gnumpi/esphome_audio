#include "AW88298.h"

#ifdef ADF_PIPELINE_I2C_IC

#include "i2s_stream_mod.h"
#include "../../adf_pipeline/adf_pipeline.h"

#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"
//#include "soc/reg.h"

namespace esphome {
using namespace esp_adf;
namespace i2s_audio {

static const char *const TAG = "adf_aw88298";

const int AW9523_REG_OUTPUT0 = 0x02;
const int AW9523_REG_OUTPUT1 = 0x03;

const int AW88298_REG_SYSCTRL   = 0x04;
const int AW88298_REG_SYSCTRL2  = 0x05;
const int AW88298_REG_I2SCTRL   = 0x06;
const int AW88298_REG_HAGCCFG3  = 0x0C;
const int AW88298_REG_BSTCTRL2  = 0x61;

void ADFI2SOut_AW88298::setup(){
    ADFElementI2SOut::setup();
    this->use_adf_alc_ = false;
}

void ADFI2SOut_AW88298::on_pipeline_status_change(){
  if( this->pipeline_->getState() == PipelineState::PREPARING ){

    i2c::I2CDevice aw9523;
    aw9523.set_i2c_bus( this->bus_);
    aw9523.set_i2c_address(0x58);

    //set AW88298 - RST
    uint8_t AW88298_RST_BIT = 2;
    uint8_t reg = aw9523.reg( AW9523_REG_OUTPUT0 ).get();
    aw9523.reg( AW9523_REG_OUTPUT0) = reg | (1 << AW88298_RST_BIT) ;
    reg = aw9523.reg( AW9523_REG_OUTPUT0 ).get();
    ESP_LOGCONFIG(TAG, "read AW9523_REG_OUTPUT0 :0x%x", reg );

    //disable boost mode
    /*
        15    not used           0
        14:12 BST_MODE         000  transparent mode
        11    not used           0
        10:8  BST_TDEG         110  352 ms
        7:6   reserved          01
        5:0   VOUT_VREFSET  110011  BOOST max output voltage control bits (125mV/Step)
    */

    uint16_t val = 0x0673;
    this->write_bytes_16(AW88298_REG_BSTCTRL2, &val, 1 );
    val = 0;
    this->read_byte_16(AW88298_REG_BSTCTRL2, &val );
    esph_log_d( TAG, "read AW88298_REG_BSTCTRL2 :0x%x", val );

    // enabled I2EN and switch to operation mode
    val = 0x4042; // I2SEN=1 AMPPD=1 PWDN=0
    this->write_bytes_16( AW88298_REG_SYSCTRL,  &val, 1);
    val = 0;
    this->read_byte_16(AW88298_REG_SYSCTRL, &val );
    esph_log_d( TAG, "read AW88298_REG_SYSCTRL :0x%x", val );

    val = 0x0008; // RMSE=0 HAGCE=0 HDCCE=0 HMUTE=0
    this->write_bytes_16( AW88298_REG_SYSCTRL2, &val, 1);
    val = 0;
    this->read_byte_16( AW88298_REG_SYSCTRL2 , &val );
    esph_log_d( TAG, "read AW88298_REG_SYSCTRL2 :0x%x", val );


    val = 0;
    this->read_byte_16( 0x01, &val );
    esph_log_d( TAG, "read AW88298_SYS_STATUS :0x%x", val );
  }
}



void debug_clk_rates(){
    uint32_t clkm_div_num = I2S0.tx_clkm_conf.tx_clkm_div_num;
    uint32_t clkm_clk_active = I2S0.tx_clkm_conf.tx_clk_active;
    uint32_t clkm_clk_sel = I2S0.tx_clkm_conf.tx_clk_sel;

    ESP_LOGI(TAG, "I2S Clock Config: clkm_div_num = %u, clkm_clk_active = %u, clkm_sel = %u",
             clkm_div_num, clkm_clk_active, clkm_clk_sel);

    uint32_t clkm_div_z = I2S0.tx_clkm_div_conf.tx_clkm_div_z;
    uint32_t clkm_div_x = I2S0.tx_clkm_div_conf.tx_clkm_div_x;
    uint32_t clkm_div_y = I2S0.tx_clkm_div_conf.tx_clkm_div_y;
    uint32_t clkm_div_yn1 = I2S0.tx_clkm_div_conf.tx_clkm_div_yn1;

    ESP_LOGI(TAG, "I2S Clock Division Parameters: z = %u, y = %u, x = %u, yn1 = %u",
             clkm_div_z, clkm_div_y, clkm_div_x, clkm_div_yn1);

    if( clkm_div_yn1 == 0){
      uint32_t b = clkm_div_z;
      uint32_t a = (clkm_div_x + 1) * b + clkm_div_y;
      ESP_LOGI(TAG, "I2S Clock Division Parameters: a = %u, b = %u", a, b);
    }


}



void ADFI2SOut_AW88298::on_settings_request(AudioPipelineSettingsRequest &request) {
  if ( !this->adf_i2s_stream_writer_ ){
    return;
  }

  bool rate_bits_channels_updated = false;
  if ( request.sampling_rate > 0 ){
    uint32_t final_rate = request.sampling_rate >= 44100 ? 48000 : 16000;
    if( final_rate != this->sample_rate_){
      this->sample_rate_ = final_rate;
      rate_bits_channels_updated = true;
    }
  }

  if(request.number_of_channels > 0 && (uint8_t) request.number_of_channels != this->channels_)
  {
    this->channels_ = request.number_of_channels;
    rate_bits_channels_updated = true;
  }

  if (request.bit_depth > 0 && (uint8_t) request.bit_depth != this->bits_per_sample_) {
    bool supported = false;
    for (auto supported_bits : this->supported_bits_per_sample_) {
      if (supported_bits == (uint8_t) request.bit_depth) {
        supported = true;
      }
    }
    if (!supported) {
      request.failed = true;
      request.failed_by = this;
      return;
    }
    this->bits_per_sample_ = request.bit_depth;
    rate_bits_channels_updated = true;
  }

  if (rate_bits_channels_updated) {

    audio_element_set_music_info(this->adf_i2s_stream_writer_,this->sample_rate_, this->channels_,
        this->bits_per_sample_ );

    if( this->pipeline_->getState() == PipelineState::STARTING &&
     this->pipeline_->getState() == PipelineState::RUNNING ){

    }
    esph_log_d( TAG, "set AW88298 to configure mode" );
    uint16_t val  = 0x4000; // I2SEN=1 AMPPD=0 PWDN=0
             val |= (1 << 6) ; // I2S Enable
             val |= (1 << 1) ; // power down amp for configuration
    this->write_bytes_16( AW88298_REG_SYSCTRL,  &val, 1);

    if (i2s_stream_set_clk(this->adf_i2s_stream_writer_, this->sample_rate_, this->bits_per_sample_,
                           this->channels_) != ESP_OK) {
      esph_log_e(TAG, "error while setting sample rate and bit depth,");
      request.failed = true;
      request.failed_by = this;
      return;
    }

    static constexpr uint8_t rate_tbl[] = {4,5,6,8,10,11,15,20,22,44};

    size_t reg0x06_value = 0;
    size_t rate = ( this->sample_rate_ + 1102) / 2205;
    while (rate > rate_tbl[reg0x06_value] && ++reg0x06_value < sizeof(rate_tbl)) {}

    reg0x06_value |= 0x1C00;  // I2SBCK=0 (BCK mode 16*2)

    this->write_bytes_16( AW88298_REG_I2SCTRL, (uint16_t *) &reg0x06_value, 1 );
    val = 0;
    this->read_byte_16(AW88298_REG_I2SCTRL, &val );
    esph_log_d( TAG, "read AW88298_REG_I2SCTRL :0x%x", val );

    val = 0;
    this->read_byte_16( 0x01, &val );
    esph_log_d( TAG, "read AW88298_SYS_STATUS :0x%x", val );


  }

  if (request.target_volume > -1) {

    uint16_t status = 0;
    this->read_byte_16( 0x01, &status );
    esph_log_d( TAG, "read AW88298_SYS_STATUS :0x%x", status );
    uint16_t systctrl = 0;
    this->read_byte_16( AW88298_REG_SYSCTRL, &systctrl );
    esph_log_d( TAG, "read AW88298_REG_SYSCTRL :0x%x", systctrl );

    if( status & 1 ){
      uint16_t val  = 0x4000; // I2SEN=1 AMPPD=0 PWDN=0
      val |= (1 << 6) ; // I2S Enable
      val |= (0 << 1) ; // power up amp after configuration
      this->write_bytes_16( AW88298_REG_SYSCTRL,  &val, 1);
    }

    //aw88298_write_reg( 0x0C, 0x0064 );  // volume setting (full volume)
    // 0 to 96 dB
    // 7:4 in unit of -6dB
    // 3:0 in unit of -0.5dB
    uint16_t val = (1. - request.target_volume) * 192.;
    val = (val / 12) << 4 | (val % 12);
    val = (val << 8 ) | 0x0064;
    this->write_bytes_16( AW88298_REG_HAGCCFG3, &val, 1 );

    if( this->use_adf_alc_ ){
      int target_volume = (int) (request.target_volume * 64.) - 32;
      if (i2s_alc_volume_set(this->adf_i2s_stream_writer_, target_volume) != ESP_OK) {
        esph_log_e(TAG, "error setting volume to %d", target_volume);
        request.failed = true;
        request.failed_by = this;
        return;
      }
    }
  }

  request.final_sampling_rate = this->sample_rate_;
  request.final_bit_depth = this->bits_per_sample_;
  request.number_of_channels = this->channels_;

}




}
}
#endif
