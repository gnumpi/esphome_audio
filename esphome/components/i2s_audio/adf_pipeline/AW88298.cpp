#include "AW88298.h"

#ifdef USE_ESP_IDF

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

    /*
    M5.begin();

        // Power Hold pin for Capsule/Dial/DinMeter
        // I think this is cam related
        // GPIO46 : CAM_VSYNC
        m5gfx::gpio_hi(GPIO_NUM_46);
        m5gfx::pinMode(GPIO_NUM_46, m5gfx::pin_mode_t::output);

        _setup_pinmap(board);

        _setup_i2c(board);

        _begin(cfg);
            Power.begin()
            Power.setExtOutput(cfg.output_power);

            /// Slightly lengthen the acceptance time of the AXP192 power button multiclick.
            BtnPWR.setHoldThresh(BtnPWR.getHoldThresh() * 1.2);

            // setup Hardware Buttons
            m5gfx::pinMode(GPIO_NUM_41, m5gfx::pin_mode_t::input);



        // Module Display / Unit OLED / Unit LCD is determined after _begin (because it must be after external power supply)
        M5ModuleDisplay dsp(cfg.module_display);
        dsp.init()
            // Wechseln Sie die Rolle von GPIO35 während des CS-Betriebs (MISO oder D/C);

                // FSPIQ_IN_IDX==FSPI MISO / SIG_GPIO_OUT_IDX==GPIO OUT
                // *(volatile uint32_t*)GPIO_FUNC35_OUT_SEL_CFG_REG = flg ? FSPIQ_OUT_IDX : SIG_GPIO_OUT_IDX;

                // Wenn CS HIGH, GPIO-Ausgabe deaktivieren und als MISO-Eingabe fungieren lassen.
                 //Wenn CS NIEDRIG ist, GPIO-Ausgang aktivieren und als D/C fungieren.


            /// Wenn der SPI-Bus-Pullup nicht wirksam ist, aktivieren Sie den VBUS 5V-Ausgang.
            /// (Denn es gibt Kombinationen wie USB-Hostmodule, die Strom aus der Signalleitung absorbieren, wenn keine 5V ausgegeben werden)
            auto result = lgfx::gpio::command(
              (const uint8_t[]) {
              lgfx::gpio::command_mode_input_pullup, GPIO_NUM_35,
              lgfx::gpio::command_mode_input_pullup, GPIO_NUM_36,
              lgfx::gpio::command_mode_input_pullup, GPIO_NUM_37,
              lgfx::gpio::command_read             , GPIO_NUM_35,
              lgfx::gpio::command_read             , GPIO_NUM_36,
              lgfx::gpio::command_read             , GPIO_NUM_37,
              lgfx::gpio::command_end
              }
            );

            uint8_t reg0x02 = (result == 0) ? 0b00000111 : 0b00000101;
            uint8_t reg0x03 = (result == 0) ? 0b10000011 : 0b00000011;
            m5gfx::i2c::bitOn(i2c_port, aw9523_i2c_addr, 0x02, reg0x02); //port0 output ctrl
            m5gfx::i2c::bitOn(i2c_port, aw9523_i2c_addr, 0x03, reg0x03); //port1 output ctrl
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x04, 0b00011000);  // CONFIG_P0
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x05, 0b00001100);  // CONFIG_P1
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x11, 0b00010000);  // GCR P0 port is Push-Pull mode.
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x12, 0b11111111);  // LEDMODE_P0
            m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x13, 0b11111111);  // LEDMODE_P1

            m5gfx::i2c::writeRegister8(i2c_port, axp_i2c_addr, 0x90, 0xBF); // LDOS ON/OFF control 0
            m5gfx::i2c::writeRegister8(i2c_port, axp_i2c_addr, 0x95, 0x28); // ALDO3 set to 3.3v // for TF card slot

        // Speaker selection is performed after the Module Display has been determined.
        _begin_spk(cfg);
            auto mic_cfg = Mic.config();
            mic_cfg.i2s_port = I2S_NUM_0;

            mic_cfg.magnification = 2;
            mic_cfg.over_sampling = 1;
            mic_cfg.pin_mck = GPIO_NUM_0;
            mic_cfg.pin_bck = GPIO_NUM_34;
            mic_cfg.pin_ws = GPIO_NUM_33;
            mic_cfg.pin_data_in = GPIO_NUM_14;
            mic_cfg.i2s_port = I2S_NUM_1;
            mic_cfg.stereo = true;

            auto spk_cfg = Speaker.config();
            spk_cfg.magnification = 16;



    auto cfg = M5.Speaker.config();
    cfg.task_priority = 15;
    M5.Speaker.config(cfg);
    M5.Speaker.begin();
        _cb_set_enabled(_cb_set_enabled_args, true)
            this->setup_aw88298

        _setup_i2s()


    M5.Speaker.setVolume(200);
    */

    i2c::I2CDevice aw9523;
    aw9523.set_i2c_bus( this->bus_);
    aw9523.set_i2c_address(0x58);

    uint8_t reg = aw9523.reg( AW9523_REG_OUTPUT1 ).get();
    aw9523.reg( AW9523_REG_OUTPUT1) = reg | 0b10000000 ;
    reg = aw9523.reg( AW9523_REG_OUTPUT1 ).get();
    ESP_LOGCONFIG(TAG, "read :0x%x", reg );

    uint8_t result = 0;
    uint8_t reg0x02 = (result == 0) ? 0b00000111 : 0b00000101;
    uint8_t reg0x03 = (result == 0) ? 0b10000011 : 0b00000011;
    aw9523.reg( 0x02 ) = reg0x02; //port0 output ctrl
    aw9523.reg( 0x03 ) = reg0x03; //port1 output ctrl

    /*
    m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x04, 0b00011000);  // CONFIG_P0
    m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x05, 0b00001100);  // CONFIG_P1
    m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x11, 0b00010000);  // GCR P0 port is Push-Pull mode.
    m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x12, 0b11111111);  // LEDMODE_P0
    m5gfx::i2c::writeRegister8(i2c_port, aw9523_i2c_addr, 0x13, 0b11111111);  // LEDMODE_P1
    */
    aw9523.reg(0x04) = 0b00011000;  // CONFIG_P0
    aw9523.reg(0x05) = 0b00001100;  // CONFIG_P1
    aw9523.reg(0x11) = 0b00010000;  // GCR P0 port is Push-Pull mode.
    aw9523.reg(0x12) = 0b11111111;  // LEDMODE_P0
    aw9523.reg(0x13) = 0b11111111;  // LEDMODE_P1

    i2c::I2CDevice axp2101;
    axp2101.set_i2c_bus( this->bus_);
    axp2101.set_i2c_address(0x34);





    static constexpr std::uint8_t reg_data_array[] =
      {
        0x90, 0xBF  // LDOS ON/OFF control 0
        //0x90, 0xBD  // LDOS ON/OFF control 0 Why can't I just switch ALDO2 it off?
      , 0x92, 18 -5 // ALDO1 set to 1.8v // for AW88298
      , 0x93, 33 -5 // ALDO2 set to 3.3v // for ES7210
      , 0x94, 33 -5 // ALDO3 set to 3.3v // for camera
      , 0x95, 33 -5 // ALDO3 set to 3.3v // for TF card slot
      , 0x27, 0x00 // PowerKey Hold=1sec / PowerOff=4sec
      , 0x69, 0x11 // CHGLED setting
      , 0x10, 0x30 // PMU common config
      };

    //Axp2101.writeRegister8Array(reg_data_array, sizeof(reg_data_array));
    for (size_t i = 0; i < sizeof(reg_data_array); i+=2)
    {
      axp2101.reg(reg_data_array[i]) = reg_data_array[i+1];
    }

    #define XPOWERS_AXP2101_LDO_ONOFF_CTRL0                  (0x90)
    reg = axp2101.reg( XPOWERS_AXP2101_LDO_ONOFF_CTRL0 ).get();
    ESP_LOGCONFIG(TAG, "read XPOWERS_AXP2101_LDO_ONOFF_CTRL0 :0x%x", reg );

    reg = axp2101.reg( 0x92 ).get();
    ESP_LOGCONFIG(TAG, "read XPOWERS_AXP2101_ALDO_VOLTAGE :0x%x", reg );

    //ES7210 ADC
    struct __attribute__((packed)) reg_data_t
    {
      uint8_t reg;
      uint8_t value;
    };
    if (false)
    {
      i2c::I2CDevice es7210;
      es7210.set_i2c_bus( this->bus_);
      es7210.set_i2c_address(0x40);
      es7210.reg(0x00) = 0xBF; // RESET_CTL
      static constexpr reg_data_t data[] =
      {
        { 0x00, 0x41 }, // RESET_CTL
        { 0x01, 0x1f }, // CLK_ON_OFF
        //{ 0x06, 0x00 }, // DIGITAL_PDN
        { 0x06, 0x01 }, // DIGITAL_PDN (disable pull down resistors)
        { 0x07, 0x20 }, // ADC_OSR
        { 0x08, 0x10 }, // MODE_CFG
        { 0x09, 0x30 }, // TCT0_CHPINI
        { 0x0A, 0x30 }, // TCT1_CHPINI
        { 0x20, 0x0a }, // ADC34_HPF2
        { 0x21, 0x2a }, // ADC34_HPF1
        { 0x22, 0x0a }, // ADC12_HPF2
        { 0x23, 0x2a }, // ADC12_HPF1
        { 0x02, 0xC1 },
        { 0x04, 0x01 },
        { 0x05, 0x00 },
        { 0x11, 0x60 },
        { 0x40, 0x42 }, // ANALOG_SYS
        { 0x41, 0x70 }, // MICBIAS12
        { 0x42, 0x70 }, // MICBIAS34
        { 0x43, 0x1B }, // MIC1_GAIN
        { 0x44, 0x1B }, // MIC2_GAIN
        { 0x45, 0x00 }, // MIC3_GAIN
        { 0x46, 0x00 }, // MIC4_GAIN
        { 0x47, 0x00 }, // MIC1_LP
        { 0x48, 0x00 }, // MIC2_LP
        { 0x49, 0x00 }, // MIC3_LP
        { 0x4A, 0x00 }, // MIC4_LP
        { 0x4B, 0x00 }, // MIC12_PDN
        { 0x4C, 0xFF }, // MIC34_PDN
        //{ 0x01, 0x14 }, // CLK_ON_OFF
        { 0x01, 0xBF }, // CLK_ON_OFF
      };
      for (auto& d: data)
      {
        es7210.reg(d.reg) = d.value;
      }
    }


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


void ADFI2SOut_AW88298::sdk_event_handler_(audio_event_iface_msg_t &msg) {
  audio_element_handle_t i2s_handler = this->adf_i2s_stream_writer_;
  if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_handler &&
      msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {

      audio_element_status_t status;
      std::memcpy(&status, &msg.data, sizeof(audio_element_status_t));
      esph_log_i(TAG, "[ %s ] status: %d", audio_element_get_tag(i2s_handler), status);
        switch (status) {
          case AEL_STATUS_STATE_RUNNING:
            //setup_aw88298_();
            break;
          default:
            break;
        }

  }
}


bool ADFI2SOut_AW88298::setup_aw88298_(){

    i2c::I2CDevice aw9523;
    aw9523.set_i2c_bus( this->bus_);
    aw9523.set_i2c_address(0x58);

    //set AW88298 - RST
    uint8_t AW88298_RST_BIT = 2;
    uint8_t reg = aw9523.reg( AW9523_REG_OUTPUT0 ).get();
    aw9523.reg( AW9523_REG_OUTPUT0) = reg | (1 << AW88298_RST_BIT) ;
    reg = aw9523.reg( AW9523_REG_OUTPUT0 ).get();
    ESP_LOGCONFIG(TAG, "read AW9523_REG_OUTPUT0 :0x%x", reg );

    /*
    // enabled I2EN and switch to operation mode
    val = 0x4040; // I2SEN=1 AMPPD=0 PWDN=0
    this->write_bytes_16( AW88298_REG_SYSCTRL,  &val, 1);
    val = 0;
    aw88298.read_byte_16(AW88298_REG_SYSCTRL, &val );
    esph_log_d( TAG, "read AW88298_REG_SYSCTRL :0x%x", val );

    val = 0;
    aw88298.read_byte_16( 0x01, &val );
    esph_log_d( TAG, "read AW88298_SYS_STATUS :0x%x", val );

    val = 0x0008; // RMSE=0 HAGCE=0 HDCCE=0 HMUTE=0
    this->write_bytes_16( AW88298_REG_SYSCTRL2, &val, 1);
    val = 0;
    aw88298.read_byte_16( AW88298_REG_SYSCTRL2 , &val );
    esph_log_d( TAG, "read AW88298_REG_SYSCTRL2 :0x%x", val );

    aw88298.read_byte_16( 0x07 , &val );
    esph_log_d( TAG, "read 0x07 :0x%x", val );


    static constexpr uint8_t rate_tbl[] = {4,5,6,8,10,11,15,20,22,44};

    size_t reg0x06_value = 0;
    size_t rate = (this->sample_rate_ + 1102) / 2205;
    while (rate > rate_tbl[reg0x06_value] && ++reg0x06_value < sizeof(rate_tbl)) {}

        15:14  reserved
        13     0: not attenuated 1: attenuated by -6dB
        12     Disable/Enable I2S receiver module

        11:10  00: reserved
               01: left
               10: right
               11: mono, (L+R)/2
        9:8    00: Philips standard I2S
               01: MSB justified
               10: LSB justified
               11: reserved

        7:6    00: 16 bit
               01: 20 bit
               10: 24 bit
               11: 32 bit
        5:4    00: 32 * fs(16*2)
               01: 48 * fs(24*2)
               10: 64 * fs(32*2)
               11: reserved

        3:0        sample rate

    reg0x06_value |= 0x1C00;  // I2SBCK=0 (BCK mode 16*2)

    this->write_bytes_16( AW88298_REG_I2SCTRL, (uint16_t *) &reg0x06_value, 1 );
    val = 0;
    aw88298.read_byte_16(AW88298_REG_I2SCTRL, &val );
    esph_log_d( TAG, "read AW88298_REG_I2SCTRL :0x%x", val );

    //aw88298_write_reg( 0x0C, 0x0064 );  // volume setting (full volume)
    // 7:4 in unit of -6dB
    // 3:0 in unit of -0.5dB
    val = 0x0030;
    this->write_bytes_16( AW88298_REG_HAGCCFG3, &val, 1 );
     */
    return true;
}

void ADFI2SOut_AW88298::on_settings_request(AudioPipelineSettingsRequest &request) {
  if ( !this->adf_i2s_stream_writer_ ){
    return;
  }

  bool rate_bits_channels_updated = false;
  if (request.sampling_rate > 0 && (uint32_t) request.sampling_rate != this->sample_rate_) {
    this->sample_rate_ = request.sampling_rate;
    rate_bits_channels_updated = true;
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

    audio_element_set_music_info(this->adf_i2s_stream_writer_,this->sample_rate_, this->channels_, this->bits_per_sample_ );

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


    return;
  }

  if (request.target_volume > -1) {

    uint16_t vbat_det = 0;
    this->read_byte_16(0x12, &vbat_det);
    esph_log_d( TAG, "read vbat_det :0x%x", vbat_det );

    uint16_t pvdd_det = 0;
    this->read_byte_16(0x14, &pvdd_det);
    esph_log_d( TAG, "read vbat_det :0x%x", pvdd_det );



    uint16_t status = 0;
    this->read_byte_16( 0x01, &status );
    esph_log_d( TAG, "read AW88298_SYS_STATUS :0x%x", status );
    uint16_t systctrl = 0;
    this->read_byte_16( AW88298_REG_SYSCTRL, &systctrl );
    esph_log_d( TAG, "read AW88298_REG_SYSCTRL :0x%x", systctrl );

    /*
    float cur_clk = i2s_get_clk(this->parent_->get_port());
    esph_log_d( TAG, "current clock : %4.2f", cur_clk );
    */

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


    if( false ){
      int target_volume = (int) (request.target_volume * 64.) - 32;
      if (i2s_alc_volume_set(this->adf_i2s_stream_writer_, target_volume) != ESP_OK) {
        esph_log_e(TAG, "error setting volume to %d", target_volume);
        request.failed = true;
        request.failed_by = this;
        return;
      }
    }
  }
}




}
}
#endif
