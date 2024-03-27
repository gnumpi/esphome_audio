#include "ES7210.h"

#ifdef ADF_PIPELINE_I2C_IC

#include "i2s_stream_mod.h"
#include "../../adf_pipeline/adf_pipeline.h"

#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"
//#include "soc/reg.h"

namespace esphome {
using namespace esp_adf;
namespace i2s_audio {

static const char *const TAG = "adf_es7210";

// ES7210 ADC Register Definitions
constexpr uint8_t ES7210_RESET_CONTROL = 0x00;
constexpr uint8_t ES7210_CLOCK_OFF = 0x01;
constexpr uint8_t ES7210_MAIN_CLOCK_CONTROL = 0x02;
constexpr uint8_t ES7210_MASTER_CLOCK_CONTROL = 0x03;
constexpr uint8_t ES7210_MASTER_LRCK_DIVIDER_1 = 0x04;
constexpr uint8_t ES7210_MASTER_LRCK_DIVIDER_0 = 0x05;
constexpr uint8_t ES7210_POWER_DOWN = 0x06;
constexpr uint8_t ES7210_ADC_OSR_CONFIG = 0x07;
constexpr uint8_t ES7210_MODE_CONFIG = 0x08;
constexpr uint8_t ES7210_TIME_CONTROL_0 = 0x09;
constexpr uint8_t ES7210_TIME_CONTROL_1 = 0x0A;
constexpr uint8_t ES7210_CHIP_STATUS = 0x0B;
constexpr uint8_t ES7210_INTERRUPT_CONTROL = 0x0C;
constexpr uint8_t ES7210_MISC_CONTROL = 0x0D;
constexpr uint8_t ES7210_DMIC_CONTROL = 0x10;
constexpr uint8_t ES7210_SDP_INTERFACE_CONFIG_1 = 0x11;
constexpr uint8_t ES7210_SDP_INTERFACE_CONFIG_2 = 0x12;
constexpr uint8_t ES7210_ADC_AUTOMUTE_CONTROL = 0x13;
constexpr uint8_t ES7210_ADC34_MUTE_CONTROL = 0x14;
constexpr uint8_t ES7210_ADC12_MUTE_CONTROL = 0x15;
constexpr uint8_t ES7210_ALC_SELECT = 0x16;
constexpr uint8_t ES7210_ALC_COMMON_CONFIG_1 = 0x17;
constexpr uint8_t ES7210_ADC34_ALC_LEVEL = 0x18;
constexpr uint8_t ES7210_ADC12_ALC_LEVEL = 0x19;
constexpr uint8_t ES7210_ALC_COMMON_CONFIG_2 = 0x1A;
constexpr uint8_t ES7210_ADC4_MAX_GAIN = 0x1B;
constexpr uint8_t ES7210_ADC3_MAX_GAIN = 0x1C;
constexpr uint8_t ES7210_ADC2_MAX_GAIN = 0x1D;
constexpr uint8_t ES7210_ADC1_MAX_GAIN = 0x1E;
constexpr uint8_t ES7210_ADC34_HPF2 = 0x20;
constexpr uint8_t ES7210_ADC34_HPF1 = 0x21;
constexpr uint8_t ES7210_ADC12_HPF1 = 0x22;
constexpr uint8_t ES7210_ADC12_HPF2 = 0x23;
constexpr uint8_t ES7210_CHIP_ID1 = 0x3D;
constexpr uint8_t ES7210_CHIP_ID0 = 0x3E;
constexpr uint8_t ES7210_CHIP_VERSION = 0x3F;
constexpr uint8_t ES7210_ANALOG_SYSTEM = 0x40;
constexpr uint8_t ES7210_MIC1_2_BIAS = 0x41;
constexpr uint8_t ES7210_MIC3_4_BIAS = 0x42;
constexpr uint8_t ES7210_MIC1_GAIN = 0x43;
constexpr uint8_t ES7210_MIC2_GAIN = 0x44;
constexpr uint8_t ES7210_MIC3_GAIN = 0x45;
constexpr uint8_t ES7210_MIC4_GAIN = 0x46;
constexpr uint8_t ES7210_MIC1_LOW_POWER = 0x47;
constexpr uint8_t ES7210_MIC2_LOW_POWER = 0x48;
constexpr uint8_t ES7210_MIC3_LOW_POWER = 0x49;
constexpr uint8_t ES7210_MIC4_LOW_POWER = 0x4A;
constexpr uint8_t ES7210_MIC1_2_POWER_DOWN = 0x4B;
constexpr uint8_t ES7210_MIC3_4_POWER_DOWN = 0x4C;


void ADFI2SIn_ES7210::setup(){
      uint8_t chip_id = 0;
      chip_id = this->reg(0x3D).get();
      if( chip_id != 0x72){
        esph_log_e(TAG, "Invalid chip id!" );
        this->mark_failed();
      }

      //try read:
      // 0x3D : CHIP ID1 [0x72]
      // 0x3E : CHIP ID0 [0x10]

      i2c::I2CDevice es7210;
      es7210.set_i2c_bus( this->bus_);
      es7210.set_i2c_address(0x40);
      es7210.reg(0x00) = 0xBF; // RESET_CTL

      struct __attribute__((packed)) reg_data_t
      {
        uint8_t reg;
        uint8_t value;
      };

      static constexpr reg_data_t data[] =
      {
        { 0x00, 0x41 }, // RESET_CTL
        { 0x01, 0x1f }, // CLK_ON_OFF

        { 0x06, 0x00 }, // DIGITAL_PDN
        //{ 0x06, 0x01 }, // DIGITAL_PDN (disable pull down resistors)

        { 0x07, 0x20 }, // ADC_OSR
        { 0x08, 0x10 }, // MODE_CFG [2ch, no_invert, EQ on, single speed, LRCK: slave mode]

        { 0x09, 0x30 }, // TCT0 CHIPINI_LGTH [48] period=CHIPINI_LGTH/(LRCK frequency)*16
        { 0x0A, 0x30 }, // TCT1 PWRUP_LGTH [48] period=PWRUP_LGTH/(LRCK frequency)*16

        //{0x0B,0x00}, // CLEAR_RAM, FORCE_CSM, ADC34_MUTE_FLAG, ADC12_MUTE_FLAG, CSM_STATE
        //{0x0C,0x00}, // Interrupt control
        //{0,0D,0x09}, // I2C_IBTHD_SEL, CLKDBLK_PW_SEL, CLKDBL_PATH_SEL, LRCK_EXTEND, DELAY_SEL

        //{0x10, 0x00}, // DMIC-Ctrl ADC34_DMIC_ON, ADC12_DMIC_ON, ADC34_DMIC_GAIN, ADC12_DMIC_GAIN


        { 0x20, 0x0a }, // ADC34_HPF2
        { 0x21, 0x2a }, // ADC34_HPF1
        { 0x22, 0x0a }, // ADC12_HPF2
        { 0x23, 0x2a }, // ADC12_HPF1

        //0x24 to 0x37 EQ coefficients

        { 0x02, 0xC1 }, // Main clk cntrl [bypass DLL, use clock doubler, CLK_ADC_DIV: no divide]
      //{ 0x03, 0x04 }, // MCL cntrl [MCLK from pad, divide by 4]
        { 0x04, 0x01 }, // Master LRCK divider 1 [ M_LRCK_DIV[11:8] ]
        { 0x05, 0x00 }, // Master LRCK divider 0 [ M_LRCK_DIV[ 7:0] ]

        { 0x11, 0x60 }, //SDP IF: SP_WL, SP_LRP, SP_PROTOCOL [16bit, L/R normal polarity, I2S]
      //{ 0x12, 0x00 }, // SDOUT_MODE: [ADC12 to SDOUT1, ADC34 to SDOUT2]
      //{ 0x13, 0x00 }, // ADC AUTO-MUTE [no auto-mute]
      //{ 0x14, 0x00 }, // ADC34 MUTE Cntrl
      //{ 0x15, 0x00 }, // ADC12 MUTE Cntrl
      //{ 0x16, 0x00 }, // Automatic Level Control (ALC) [ALC12 and ALC34 off]
      //{ 0x17, 0x00 }, // ALC common cfg
      //{ 0x18, 0xF7 }, // ADC34 ALC LEVEL
      //{ 0x19, 0xF7 }, // ADC12 ALC LEVEL
      //{ 0x1A, 0x00 }, // ALC common cfg 2

        { 0x40, 0x42 }, // ANALOG_SYS [ VX2OFF: turn off (VDDA=3.3V), VMIDSEL: 500 kΩ divider enabled]
        { 0x41, 0x70 }, // MICBIAS12 [LVL_MICBIAS12: 2.87V (VDDM=3.3V), ADC12BIAS_SWH: x1]
        { 0x42, 0x70 }, // MICBIAS34 [LVL_MICBIAS34: 2.87V (VDDM=3.3V), ADC34BIAS_SWH: x1]
        { 0x43, 0x1B }, // MIC1_GAIN [SELMIC1: select MIC1P and MIC1N, GAIN: 33dB]
        { 0x44, 0x1B }, // MIC2_GAIN [SELMIC2: select MIC2P and MIC2N, GAIN: 33dB]
        { 0x45, 0x00 }, // MIC3_GAIN [deselect MIC3]
        { 0x46, 0x00 }, // MIC4_GAIN [deselect MIC4]
        { 0x47, 0x00 }, // MIC1 Low Power [all set to normal]
        { 0x48, 0x00 }, // MIC2 Low Power [all set to normal]
        { 0x49, 0x00 }, // MIC3 Low Power [all set to normal]
        { 0x4A, 0x00 }, // MIC4 Low Power [all set to normal]
        { 0x4B, 0x00 }, // MIC 1/2 power down [all set to normal]
        { 0x4C, 0xFF }, // MIC3 3/4 power down [all set to power down]

        { 0x01, 0x14 }, // CLK_ON_OFF
        //{ 0x01, 0xBF }, // CLK_ON_OFF
      };
      for (auto& d: data)
      {
        es7210.reg(d.reg) = d.value;
      }
}


void ADFI2SIn_ES7210::dump_config(){
    uint8_t mode_cfg = this->reg(ES7210_MODE_CONFIG).get();
    esph_log_config(TAG, "Register: ");
    esph_log_config(TAG, "   ES7210_MODE_CONFIG: %d", mode_cfg);

}


}
}

#endif
