#pragma once

#include "esphome/components/spi/spi.h"

namespace esphome {
namespace matrixio {

/*
FPGA_SPI_CS:   GPIO_23
FPGA_SPI_MOSI: GPIO_33
FPGA_SPI_MISO: GPIO_21
FPGA_SPI_SCLK: GPIO_32
*/

const uint16_t CONF_BASE_ADDRESS = 0x0000;
const uint32_t MATRIX_CREATOR_ID = 0x05C344E8;
const uint32_t MATRIX_VOICE_ID = 0x6032BAD2;

struct hardware_address {
  uint8_t readnwrite : 1;
  uint16_t reg : 15;
};

class WishboneBus : public Component,
                    public spi::SPIDevice <spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_HIGH,
                                          spi::CLOCK_PHASE_TRAILING, spi::DATA_RATE_8MHZ> {
  public:
    void setup();
    void dump_config();
  
    void reg_write(uint16_t add, uint16_t data);
    void reg_read(uint16_t add, uint16_t* data);

    void read(uint16_t add, uint8_t* data, int length);
    void write(uint16_t add, const uint8_t* data, int length);

    void lock() { this->lock_.lock(); }
    bool try_lock() { return this->lock_.try_lock(); }
    void unlock() { this->lock_.unlock(); }

  private:    
    void read_fpga_firmware_version();
    
    uint32_t matrixio_id{0};
    uint32_t matrixio_version{0};

    Mutex lock_;
    size_t buffer_size{};
    uint8_t *buffer{nullptr};
};


class WishboneDevice {
protected:  
  WishboneDevice( uint32_t wb_base_addr ) : wb_base_address_(wb_base_addr) {} 

public:
  void set_wishbone_bus(WishboneBus* wb ){ this->wb_ = wb; }

protected:
  void wb_write(uint8_t* data, int length){this->wb_->write( this->wb_base_address_, data, length );}
  void wb_read(uint8_t* data, int length){this->wb_->read( this->wb_base_address_, data, length );}
  void reg_write(uint16_t offset, uint16_t data){this->wb_->reg_write(this->wb_base_address_ + offset, data);}
  void reg_read(uint16_t offset, uint16_t* data){this->wb_->reg_read(this->wb_base_address_ + offset, data);}
  void conf_write(uint16_t offset, uint16_t data){this->wb_->reg_write(CONF_BASE_ADDRESS + offset, data);}
  void conf_read(uint16_t offset, uint16_t* data){this->wb_->reg_read(CONF_BASE_ADDRESS + offset, data);}
   
private:
  WishboneBus *wb_{nullptr};
  const uint32_t wb_base_address_;
};


} // namespace matrixio
} //namespace esphome

