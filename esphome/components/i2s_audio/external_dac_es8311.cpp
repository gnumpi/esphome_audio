#include "external_dac.h"

#ifdef I2S_EXTERNAL_DAC

#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace i2s_audio {

static const char *const TAG = "DAC_ES8311";

struct ES8311Coefficient {
  uint32_t mclk;     // mclk frequency
  uint32_t rate;     // sample rate
  uint8_t pre_div;   // the pre divider with range from 1 to 8
  uint8_t pre_mult;  // the pre multiplier with x1, x2, x4 and x8 selection
  uint8_t adc_div;   // adcclk divider
  uint8_t dac_div;   // dacclk divider
  uint8_t fs_mode;   // single speed (0) or double speed (1)
  uint8_t lrck_h;    // adc lrck divider and dac lrck divider
  uint8_t lrck_l;    //
  uint8_t bclk_div;  // sclk divider
  uint8_t adc_osr;   // adc osr
  uint8_t dac_osr;   // dac osr
};


// ES8311 register addresses
static const uint8_t ES8311_REG00_RESET = 0x00;        // Reset
static const uint8_t ES8311_REG01_CLK_MANAGER = 0x01;  // Clock Manager: select clk src for mclk, enable clock for codec
static const uint8_t ES8311_REG02_CLK_MANAGER = 0x02;  // Clock Manager: clk divider and clk multiplier
static const uint8_t ES8311_REG03_CLK_MANAGER = 0x03;  // Clock Manager: adc fsmode and osr
static const uint8_t ES8311_REG04_CLK_MANAGER = 0x04;  // Clock Manager: dac osr
static const uint8_t ES8311_REG05_CLK_MANAGER = 0x05;  // Clock Manager: clk divider for adc and dac
static const uint8_t ES8311_REG06_CLK_MANAGER = 0x06;  // Clock Manager: bclk inverter BIT(5) and divider
static const uint8_t ES8311_REG07_CLK_MANAGER = 0x07;  // Clock Manager: tri-state, lrck divider
static const uint8_t ES8311_REG08_CLK_MANAGER = 0x08;  // Clock Manager: lrck divider
static const uint8_t ES8311_REG09_SDPIN = 0x09;        // Serial Digital Port: DAC
static const uint8_t ES8311_REG0A_SDPOUT = 0x0A;       // Serial Digital Port: ADC
static const uint8_t ES8311_REG0B_SYSTEM = 0x0B;       // System
static const uint8_t ES8311_REG0C_SYSTEM = 0x0C;       // System
static const uint8_t ES8311_REG0D_SYSTEM = 0x0D;       // System: power up/down
static const uint8_t ES8311_REG0E_SYSTEM = 0x0E;       // System: power up/down
static const uint8_t ES8311_REG0F_SYSTEM = 0x0F;       // System: low power
static const uint8_t ES8311_REG10_SYSTEM = 0x10;       // System
static const uint8_t ES8311_REG11_SYSTEM = 0x11;       // System
static const uint8_t ES8311_REG12_SYSTEM = 0x12;       // System: Enable DAC
static const uint8_t ES8311_REG13_SYSTEM = 0x13;       // System
static const uint8_t ES8311_REG14_SYSTEM = 0x14;       // System: select DMIC, select analog pga gain
static const uint8_t ES8311_REG15_ADC = 0x15;          // ADC: adc ramp rate, dmic sense
static const uint8_t ES8311_REG16_ADC = 0x16;          // ADC
static const uint8_t ES8311_REG17_ADC = 0x17;          // ADC: volume
static const uint8_t ES8311_REG18_ADC = 0x18;          // ADC: alc enable and winsize
static const uint8_t ES8311_REG19_ADC = 0x19;          // ADC: alc maxlevel
static const uint8_t ES8311_REG1A_ADC = 0x1A;          // ADC: alc automute
static const uint8_t ES8311_REG1B_ADC = 0x1B;          // ADC: alc automute, adc hpf s1
static const uint8_t ES8311_REG1C_ADC = 0x1C;          // ADC: equalizer, hpf s2
static const uint8_t ES8311_REG1D_ADCEQ = 0x1D;        // ADCEQ: equalizer B0
static const uint8_t ES8311_REG1E_ADCEQ = 0x1E;        // ADCEQ: equalizer B0
static const uint8_t ES8311_REG1F_ADCEQ = 0x1F;        // ADCEQ: equalizer B0
static const uint8_t ES8311_REG20_ADCEQ = 0x20;        // ADCEQ: equalizer B0
static const uint8_t ES8311_REG21_ADCEQ = 0x21;        // ADCEQ: equalizer A1
static const uint8_t ES8311_REG22_ADCEQ = 0x22;        // ADCEQ: equalizer A1
static const uint8_t ES8311_REG23_ADCEQ = 0x23;        // ADCEQ: equalizer A1
static const uint8_t ES8311_REG24_ADCEQ = 0x24;        // ADCEQ: equalizer A1
static const uint8_t ES8311_REG25_ADCEQ = 0x25;        // ADCEQ: equalizer A2
static const uint8_t ES8311_REG26_ADCEQ = 0x26;        // ADCEQ: equalizer A2
static const uint8_t ES8311_REG27_ADCEQ = 0x27;        // ADCEQ: equalizer A2
static const uint8_t ES8311_REG28_ADCEQ = 0x28;        // ADCEQ: equalizer A2
static const uint8_t ES8311_REG29_ADCEQ = 0x29;        // ADCEQ: equalizer B1
static const uint8_t ES8311_REG2A_ADCEQ = 0x2A;        // ADCEQ: equalizer B1
static const uint8_t ES8311_REG2B_ADCEQ = 0x2B;        // ADCEQ: equalizer B1
static const uint8_t ES8311_REG2C_ADCEQ = 0x2C;        // ADCEQ: equalizer B1
static const uint8_t ES8311_REG2D_ADCEQ = 0x2D;        // ADCEQ: equalizer B2
static const uint8_t ES8311_REG2E_ADCEQ = 0x2E;        // ADCEQ: equalizer B2
static const uint8_t ES8311_REG2F_ADCEQ = 0x2F;        // ADCEQ: equalizer B2
static const uint8_t ES8311_REG30_ADCEQ = 0x30;        // ADCEQ: equalizer B2
static const uint8_t ES8311_REG31_DAC = 0x31;          // DAC: mute
static const uint8_t ES8311_REG32_DAC = 0x32;          // DAC: volume
static const uint8_t ES8311_REG33_DAC = 0x33;          // DAC: offset
static const uint8_t ES8311_REG34_DAC = 0x34;          // DAC: drc enable, drc winsize
static const uint8_t ES8311_REG35_DAC = 0x35;          // DAC: drc maxlevel, minilevel
static const uint8_t ES8311_REG36_DAC = 0x36;          // DAC
static const uint8_t ES8311_REG37_DAC = 0x37;          // DAC: ramprate
static const uint8_t ES8311_REG38_DACEQ = 0x38;        // DACEQ: equalizer B0
static const uint8_t ES8311_REG39_DACEQ = 0x39;        // DACEQ: equalizer B0
static const uint8_t ES8311_REG3A_DACEQ = 0x3A;        // DACEQ: equalizer B0
static const uint8_t ES8311_REG3B_DACEQ = 0x3B;        // DACEQ: equalizer B0
static const uint8_t ES8311_REG3C_DACEQ = 0x3C;        // DACEQ: equalizer B1
static const uint8_t ES8311_REG3D_DACEQ = 0x3D;        // DACEQ: equalizer B1
static const uint8_t ES8311_REG3E_DACEQ = 0x3E;        // DACEQ: equalizer B1
static const uint8_t ES8311_REG3F_DACEQ = 0x3F;        // DACEQ: equalizer B1
static const uint8_t ES8311_REG40_DACEQ = 0x40;        // DACEQ: equalizer A1
static const uint8_t ES8311_REG41_DACEQ = 0x41;        // DACEQ: equalizer A1
static const uint8_t ES8311_REG42_DACEQ = 0x42;        // DACEQ: equalizer A1
static const uint8_t ES8311_REG43_DACEQ = 0x43;        // DACEQ: equalizer A1
static const uint8_t ES8311_REG44_GPIO = 0x44;         // GPIO: dac2adc for test
static const uint8_t ES8311_REG45_GP = 0x45;           // GPIO: GP control
static const uint8_t ES8311_REGFA_I2C = 0xFA;          // I2C: reset registers
static const uint8_t ES8311_REGFC_FLAG = 0xFC;         // Flag
static const uint8_t ES8311_REGFD_CHD1 = 0xFD;         // Chip: ID1
static const uint8_t ES8311_REGFE_CHD2 = 0xFE;         // Chip: ID2
static const uint8_t ES8311_REGFF_CHVER = 0xFF;        // Chip: Version

// ES8311 clock divider coefficients
static const ES8311Coefficient ES8311_COEFFICIENTS[] = {
    // clang-format off

  //   mclk,  rate, pre_  pre_  adc_  dac_  fs_   lrck  lrck bclk_  adc_  dac_
  //                 div, mult,  div,  div, mode,   _h,   _l,  div,  osr,  osr

  // 8k
  {12288000,  8000, 0x06, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  {18432000,  8000, 0x03, 0x02, 0x03, 0x03, 0x00, 0x05, 0xff, 0x18, 0x10, 0x20},
  {16384000,  8000, 0x08, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 8192000,  8000, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 6144000,  8000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 4096000,  8000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 3072000,  8000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 2048000,  8000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 1536000,  8000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 1024000,  8000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},

  // 11.025k
  {11289600, 11025, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 5644800, 11025, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 2822400, 11025, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 1411200, 11025, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},

  // 12k
  {12288000, 12000, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 6144000, 12000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 3072000, 12000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 1536000, 12000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},

  // 16k
  {12288000, 16000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  {18432000, 16000, 0x03, 0x02, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x20},
  {16384000, 16000, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 8192000, 16000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 6144000, 16000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 4096000, 16000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 3072000, 16000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 2048000, 16000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 1536000, 16000, 0x03, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
  { 1024000, 16000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},

  // 22.05k
  {11289600, 22050, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 5644800, 22050, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 2822400, 22050, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 1411200, 22050, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

  // 24k
  {12288000, 24000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  {18432000, 24000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 6144000, 24000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 3072000, 24000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 1536000, 24000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

  // 32k
  {12288000, 32000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  {18432000, 32000, 0x03, 0x04, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
  {16384000, 32000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 8192000, 32000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 6144000, 32000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 4096000, 32000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 3072000, 32000, 0x03, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 2048000, 32000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 1536000, 32000, 0x03, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
  { 1024000, 32000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

  // 44.1k
  {11289600, 44100, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 5644800, 44100, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 2822400, 44100, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 1411200, 44100, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

  // 48k
  {12288000, 48000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  {18432000, 48000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 6144000, 48000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 3072000, 48000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 1536000, 48000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

  // 64k
  {12288000, 64000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  {18432000, 64000, 0x03, 0x04, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
  {16384000, 64000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 8192000, 64000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 6144000, 64000, 0x01, 0x04, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
  { 4096000, 64000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 3072000, 64000, 0x01, 0x08, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
  { 2048000, 64000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 1536000, 64000, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0xbf, 0x03, 0x18, 0x18},
  { 1024000, 64000, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

  // 88.2k
  {11289600, 88200, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 5644800, 88200, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 2822400, 88200, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 1411200, 88200, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

  // 96k
  {12288000, 96000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  {18432000, 96000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 6144000, 96000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 3072000, 96000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
  { 1536000, 96000, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

    // clang-format on
};

static const ES8311Coefficient *get_coefficient(uint32_t mclk, uint32_t rate) {
  for (const auto &coefficient : ES8311_COEFFICIENTS) {
    if (coefficient.mclk == mclk && coefficient.rate == rate)
      return &coefficient;
  }
  return nullptr;
}

bool ES8311::init_device(){
  uint8_t chd1 = this->reg(ES8311_REGFD_CHD1).get();
  uint8_t chd2 = this->reg(ES8311_REGFE_CHD2).get();
  if( chd1 == 0x83 && chd2 == 0x11 ){
    ESP_LOGD(TAG, "ES8311 chip found.");
  }
  else
  {
    ESP_LOGD(TAG, "ES8311 chip not found.");
    return false;
  }

  // Reset
  this->reg(ES8311_REG00_RESET) = 0x1F;
  delay(20);
  this->reg(ES8311_REG00_RESET) = 0x80;

  /*
   PDN_DAC = enable DAC
   ENREFR = disable internal reference circuits for DAC output
   */
   this->reg(ES8311_REG12_SYSTEM) = 0x00;


  if( this->enable_pin_ != nullptr ){
    this->enable_pin_->setup();
    this->enable_pin_->digital_write(true);
  }
  return true;
}


bool ES8311::apply_i2s_settings(const i2s_driver_config_t&  i2s_cfg){
  ESP_LOGD(TAG, "Setting up ES8311...");

  if( (i2s_cfg.mode & I2S_MODE_SLAVE) ){
    ESP_LOGE(TAG, "Current ES8311 implementation expects an external main clock from ESP." );
    return false;
  }

  const int mclk_multiple  = i2s_cfg.mclk_multiple == 0 ? 256 : i2s_cfg.mclk_multiple;
  const int mclk_frequency = i2s_cfg.sample_rate * mclk_multiple;

  uint8_t reg01 = 0x3F;  // Enable all clocks
  this->reg(ES8311_REG01_CLK_MANAGER) = reg01;

  // Get clock coefficients from coefficient table
  auto *coefficient = get_coefficient(mclk_frequency, i2s_cfg.sample_rate);
  if (coefficient == nullptr) {
    ESP_LOGE(TAG, "Unable to configure sample rate %dHz with %dHz MCLK", i2s_cfg.sample_rate,
             mclk_frequency);
    return false;
  }

  // Register 0x02
  uint8_t reg02 = this->reg(ES8311_REG02_CLK_MANAGER).get();
  reg02 &= 0x07;
  reg02 |= (coefficient->pre_div - 1) << 5;
  reg02 |= coefficient->pre_mult << 3;
  this->reg(ES8311_REG02_CLK_MANAGER) = reg02;

  // Register 0x03
  const uint8_t reg03 = (coefficient->fs_mode << 6) | coefficient->adc_osr;
  this->reg(ES8311_REG03_CLK_MANAGER) = reg03;

  // Register 0x04
  this->reg(ES8311_REG04_CLK_MANAGER) = coefficient->dac_osr;

  // Register 0x05
  const uint8_t reg05 = ((coefficient->adc_div - 1) << 4) | (coefficient->dac_div - 1);
  this->reg(ES8311_REG05_CLK_MANAGER) = reg05;

  // Register 0x06
  uint8_t reg06 = this->reg(ES8311_REG06_CLK_MANAGER).get();
  reg06 &= ~BIT(5);
  reg06 &= 0xE0;
  if (coefficient->bclk_div < 19) {
    reg06 |= (coefficient->bclk_div - 1) << 0;
  } else {
    reg06 |= (coefficient->bclk_div) << 0;
  }
  this->reg(ES8311_REG06_CLK_MANAGER) = reg06;

  // Register 0x07
  uint8_t reg07 = this->reg(ES8311_REG07_CLK_MANAGER).get();
  reg07 &= 0xC0;
  reg07 |= coefficient->lrck_h << 0;
  this->reg(ES8311_REG07_CLK_MANAGER) = reg07;

  // Register 0x08
  this->reg(ES8311_REG08_CLK_MANAGER) = coefficient->lrck_l;

  // Register 0x09
  uint8_t reg09 = this->reg(ES8311_REG09_SDPIN).get();
  reg09 &= 0xFC; // set SDP_IN_FMT to I2S serial audio data format (0)

  //bits_per_sample
  static constexpr uint8_t supported_bps[] = {24,20,18,16,32};
  uint8_t bps = (uint8_t) i2s_cfg.bits_per_sample;
  uint8_t bps_idx = 0;
  while( bps != supported_bps[bps_idx] && ++bps_idx < sizeof(supported_bps)) {}
  if( bps_idx == sizeof(supported_bps))
  {
    ESP_LOGE(TAG, "Unsupported bits per sample: %d", bps);
    return false;
  }
  reg09 &= 0xE3;
  reg09 |= (bps_idx) << 2;
  this->reg(ES8311_REG09_SDPIN) = reg09;

  this->reg(ES8311_REG0D_SYSTEM) = 0x01;  // Power up analog circuitry
  this->reg(ES8311_REG0E_SYSTEM) = 0x02;  // Enable analog PGA, enable ADC modulator
  this->reg(ES8311_REG12_SYSTEM) = 0x00;  // Power up DAC
  this->reg(ES8311_REG13_SYSTEM) = 0x10;  // Enable output to HP drive
  this->reg(ES8311_REG1C_ADC) = 0x6A;     // ADC Equalizer bypass, cancel DC offset in digital domain
  this->reg(ES8311_REG37_DAC) = 0x08;     // Bypass DAC equalizer
  this->reg(ES8311_REG00_RESET) = 0x80;   // Power On

  this->set_volume(0.75);

  return true;
}

bool ES8311::set_mute_audio( bool mute ){
  uint8_t regv;
  ESP_LOGI(TAG, "Enter into es8311_mute(), mute = %d\n", mute);
  regv = this->reg(ES8311_REG31_DAC).get() & 0x9f;
  if (mute) {
      this->write_byte(ES8311_REG31_DAC, regv | 0x60);
  } else {
      this->write_byte(ES8311_REG31_DAC, regv);
  }
return true;
}

bool ES8311::set_volume( float volume ){
  volume = clamp(volume, 0.0f, 1.0f);
  uint8_t reg32 = remap<uint8_t, float>(volume, 0.0f, 1.0f, 0, 200);
  this->reg(ES8311_REG32_DAC) = reg32;
  return true;
}



}  // namespace i2s_audio
}  // namespace esphome

#endif
