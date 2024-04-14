#include "external_adc.h"

#ifdef I2S_EXTERNAL_ADC

#include "esphome/core/log.h"

namespace esphome {
namespace i2s_audio {

static const char *const TAG = "i2s_external_adc";

bool ES7210::init_device(){
    esph_log_d(TAG, "Init ES7210...");
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
        this->reg(d.reg) = d.value;
      }
      return true;
}

bool ES7210::apply_i2s_settings(const i2s_driver_config_t&  i2s_cfg){
    //hard coded to 16bit,
    return true;
}




}
}
#endif
