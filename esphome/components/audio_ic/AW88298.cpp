#include "AW88298.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include <soc/io_mux_reg.h>

namespace esphome {
namespace audio_ic {

static const char *TAG = "AW88298";

void AW88298Component::setup() {
    /*
        self->In_I2C.bitOn(aw9523_i2c_addr, 0x02, 0b00000100, 400000);
        static constexpr uint8_t rate_tbl[] = {4,5,6,8,10,11,15,20,22,44};
        size_t reg0x06_value = 0;
        size_t rate = (cfg.sample_rate + 1102) / 2205;
        while (rate > rate_tbl[reg0x06_value] && ++reg0x06_value < sizeof(rate_tbl)) {}

        reg0x06_value |= 0x14C0;  // I2SBCK=0 (BCK mode 16*2)
        aw88298_write_reg( 0x61, 0x0673 );  // boost mode disabled
        aw88298_write_reg( 0x04, 0x4040 );  // I2SEN=1 AMPPD=0 PWDN=0
        aw88298_write_reg( 0x05, 0x0008 );  // RMSE=0 HAGCE=0 HDCCE=0 HMUTE=0
        aw88298_write_reg( 0x06, reg0x06_value );
        aw88298_write_reg( 0x0C, 0x0064 );  // volume setting (full volume)
    */

    // 16, 20, 24, or 32 bits
    //8 kHz, 11.025 kHz, 12 kHz, 16 kHz, 22.05 kHz, 24 kHz, 32 kHz, 44.1 kHz, 48 kHz and 96 kHz

    const int AW9523_REG_OUTPUT0 = 0x02;
    const int AW9523_REG_OUTPUT1 = 0x03;
    i2c::I2CDevice aw9523;
    aw9523.set_i2c_bus( this->bus_);
    aw9523.set_i2c_address(0x58);

    uint8_t reg = aw9523.reg( AW9523_REG_OUTPUT1 ).get();
    aw9523.reg( AW9523_REG_OUTPUT1) = reg | 0b10000000 ;
    reg = aw9523.reg( AW9523_REG_OUTPUT1 ).get();
    ESP_LOGCONFIG(TAG, "read :0x%x", reg );

    i2c::I2CDevice axp2101;
    axp2101.set_i2c_bus( this->bus_);
    axp2101.set_i2c_address(0x34);

    static constexpr std::uint8_t reg_data_array[] =
      { 0x90, 0xBF  // LDOS ON/OFF control 0
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

    /*
    #define XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL                (0x80)
    #define XPOWERS_AXP2101_DC_FORCE_PWM_CTRL                (0x81)
    #define XPOWERS_AXP2101_DC_VOL0_CTRL                     (0x82)
    #define XPOWERS_AXP2101_DC_VOL1_CTRL                     (0x83)
    #define XPOWERS_AXP2101_DC_VOL2_CTRL                     (0x84)
    #define XPOWERS_AXP2101_DC_VOL3_CTRL                     (0x85)
    #define XPOWERS_AXP2101_DC_VOL4_CTRL                     (0x86)

    bool enableDC2(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 1);
    }

    #define XPOWERS_AXP2101_LDO_ONOFF_CTRL0                  (0x90)
    #define XPOWERS_AXP2101_LDO_ONOFF_CTRL1                  (0x91)
    #define XPOWERS_AXP2101_LDO_VOL0_CTRL                    (0x92)
    #define XPOWERS_AXP2101_LDO_VOL1_CTRL                    (0x93)
    #define XPOWERS_AXP2101_LDO_VOL2_CTRL                    (0x94)
    #define XPOWERS_AXP2101_LDO_VOL3_CTRL                    (0x95)
    #define XPOWERS_AXP2101_LDO_VOL4_CTRL                    (0x96)
    #define XPOWERS_AXP2101_LDO_VOL5_CTRL                    (0x97)
    #define XPOWERS_AXP2101_LDO_VOL6_CTRL                    (0x98)
    #define XPOWERS_AXP2101_LDO_VOL7_CTRL                    (0x99)
    #define XPOWERS_AXP2101_LDO_VOL8_CTRL                    (0x9A)

bool enableALDO1(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 0);
    }

    bool disableALDO1(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 0);
    }

    bool setALDO1Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_ALDO1_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_ALDO1_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_ALDO1_VOL_MIN) {
            log_e("Mistake ! ALDO1 minimum output voltage is  %umV", XPOWERS_AXP2101_ALDO1_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_ALDO1_VOL_MAX) {
            log_e("Mistake ! ALDO1 maximum output voltage is  %umV", XPOWERS_AXP2101_ALDO1_VOL_MAX);
            return false;
        }
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL0_CTRL) & 0xE0;
        val |= (millivolt - XPOWERS_AXP2101_ALDO1_VOL_MIN) / XPOWERS_AXP2101_ALDO1_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP2101_LDO_VOL0_CTRL, val);
    }
    */

}

 void AW88298Component::setup_aw88298(){}
    i2c::I2CDevice aw9523;
    aw9523.set_i2c_bus( this->bus_);
    aw9523.set_i2c_address(0x58);

    //set AW88298 - RST
    uint8_t AW88298_RST_BIT = 2;
    uint8_t reg = aw9523.reg( AW9523_REG_OUTPUT0 ).get();
    aw9523.reg( AW9523_REG_OUTPUT0) = reg | (1 << AW88298_RST_BIT) ;
    reg = aw9523.reg( AW9523_REG_OUTPUT0 ).get();
    ESP_LOGCONFIG(TAG, "read AW9523_REG_OUTPUT0 :0x%x", reg );



    const int AW88298_REG_SYSCTRL   = 0x04;
    const int AW88298_REG_SYSCTRL2  = 0x05;
    const int AW88298_REG_I2SCTRL   = 0x06;
    const int AW88298_REG_HAGCCFG3  = 0x0C;
    const int AW88298_REG_BSTCTRL2  = 0x61;

    //disable boost mode
    /*
        15    not used           0
        14:12 BST_MODE         000  transparent mode
        11    not used           0
        10:8  BST_TDEG         110  352 ms
        7:6   reserved          01
        5:0   VOUT_VREFSET  110011  BOOST max output voltage control bits (125mV/Step)
    */

    i2c::I2CDevice aw88298;
    aw88298.set_i2c_bus( this->bus_);
    aw88298.set_i2c_address(0x36);


    uint16_t val = 0x0673;
    aw88298.write_bytes_16(AW88298_REG_BSTCTRL2, &val, 1 );
    val = 0;
    aw88298.read_byte_16(AW88298_REG_BSTCTRL2, &val );
    esph_log_d( TAG, "read AW88298_REG_BSTCTRL2 :0x%x", val );

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
    size_t rate = (44100 + 1102) / 2205;
    while (rate > rate_tbl[reg0x06_value] && ++reg0x06_value < sizeof(rate_tbl)) {}
    /*
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
     */
    reg0x06_value |= 0x1C00;  // I2SBCK=0 (BCK mode 16*2)

    this->write_bytes_16( AW88298_REG_I2SCTRL, (uint16_t *) &reg0x06_value, 1 );
    val = 0;
    aw88298.read_byte_16(AW88298_REG_I2SCTRL, &val );
    esph_log_d( TAG, "read AW88298_REG_I2SCTRL :0x%x", val );

    //aw88298_write_reg( 0x0C, 0x0064 );  // volume setting (full volume)
    // 7:4 in unit of -6dB
    // 3:0 in unit of -0.5dB
    val = 0x0064;
    this->write_bytes_16( AW88298_REG_HAGCCFG3, &val, 1 );


}

}
}
