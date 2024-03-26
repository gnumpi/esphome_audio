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

/*
static constexpr reg_data_t data[] =
{
    { 0x00, 0x41 }, // Reset Control: Soft reset of the chip to ensure starting from a known state.
    { 0x01, 0x1f }, // Clock On/Off Control: Initially enables all internal clocks before configuration.
    { 0x06, 0x01 }, // Digital Power Down: Disables internal pull-down resistors to optimize power consumption.
    { 0x07, 0x20 }, // ADC OSR (Oversampling Rate): Sets a specific oversampling rate for improved noise performance.
    { 0x08, 0x10 }, // Mode Configuration: Selects the operational mode of the device (e.g., mono, stereo).
    { 0x09, 0x30 }, // Time Control 0 for Chip Initialization: Configures timing for the initialization sequence.
    { 0x0A, 0x30 }, // Time Control 1 for Chip Initialization: Further timing configuration for initialization.
    { 0x20, 0x0a }, // ADC34 High-Pass Filter 2: Configures the high-pass filter for ADC channels 3 and 4.
    { 0x21, 0x2a }, // ADC34 High-Pass Filter 1: Fine-tunes the high-pass filter settings for channels 3 and 4.
    { 0x22, 0x0a }, // ADC12 High-Pass Filter 2: Configures the high-pass filter for ADC channels 1 and 2.
    { 0x23, 0x2a }, // ADC12 High-Pass Filter 1: Fine-tunes the high-pass filter settings for channels 1 and 2.
    { 0x02, 0xC1 }, // Main Clock Control: Adjusts main clock features, possibly enabling a clock multiplier.
    { 0x04, 0x01 }, // Master LRCK Divider: Sets the LRCK divider, affecting the audio sample rate.
    { 0x05, 0x00 }, // Master LRCK Divider (continued): Completes LRCK division configuration.
    { 0x11, 0x60 }, // SDP Interface Config: Configures the Serial Data Port for audio data transmission.
    { 0x40, 0x42 }, // Analog System Configuration: Adjusts the analog system settings for optimal performance.
    { 0x41, 0x70 }, // MIC1/2 Bias Voltage: Sets the bias voltage for microphones 1 and 2, optimizing their performance.
    { 0x42, 0x70 }, // MIC3/4 Bias Voltage: Sets the bias voltage for microphones 3 and 4.
    { 0x43, 0x1B }, // MIC1 Gain: Configures the gain for microphone 1, affecting the amplitude of the captured signal.
    { 0x44, 0x1B }, // MIC2 Gain: Configures the gain for microphone 2.
    { 0x45, 0x00 }, // MIC3 Gain: Sets microphone 3 to default gain, indicating no additional amplification.
    { 0x46, 0x00 }, // MIC4 Gain: Sets microphone 4 to default gain.
    { 0x47, 0x00 }, // MIC1 Low Power: Disables low power mode for microphone 1, ensuring full performance.
    { 0x48, 0x00 }, // MIC2 Low Power: Disables low power mode for microphone 2.
    { 0x49, 0x00 }, // MIC3 Low Power: Disables low power mode for microphone 3.
    { 0x4A, 0x00 }, // MIC4 Low Power: Disables low power mode for microphone 4.
    { 0x4B, 0x00 }, // MIC1/2 Power Down: Ensures microphones 1 and 2 are powered and ready for use.
    { 0x4C, 0xFF }, // MIC3/4 Power Down: Powers down microphones 3 and 4 to conserve energy.
    { 0x01, 0xBF }, // Clock On/Off Control (updated): Adjusts clock settings after initial configuration for optimized operation.
};
*/

}
}

#endif
