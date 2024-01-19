#include "wishbone.h"
#include "esphome/core/log.h"

namespace esphome {
namespace matrixio {

static const char *const TAG = "matrixio";
static const size_t MAX_TRANSFER_SIZE = 4092;  // dictated by ESP-IDF API.

static uint8_t global_tx_buffer[MAX_TRANSFER_SIZE];

void WishboneBus::setup() {
  ESP_LOGD(TAG, "Setting up Matrixio...");
  this->spi_setup();
  this->buffer = global_tx_buffer;
  this->buffer_size = MAX_TRANSFER_SIZE;
  this->read_fpga_firmware_version();

  if (this->matrixio_id != MATRIX_CREATOR_ID && this->matrixio_id != MATRIX_VOICE_ID ){
    ESP_LOGE(TAG, "Didn't find any MatrixIO device. Check SPI setup.");
    this->mark_failed();
  }
}

void WishboneBus::dump_config(){
  esph_log_config(TAG, "Matrixio:");
    esph_log_config(TAG, "  Data rate: %uMHz", (unsigned) (this->data_rate_ / 1000000));
    LOG_PIN("  CS pin: ", this->cs_);
    if (this->matrixio_id == MATRIX_VOICE_ID){
    esph_log_config(TAG, "  Found Matrixio Voice with firmware: %lu", this->matrixio_version );
  }
  else if (this->matrixio_id == MATRIX_CREATOR_ID){
    esph_log_config(TAG, "  Found Matrixio Creator with firmware: %lu", this->matrixio_version );
  }
  else {
    esph_log_config(TAG, "  Matrixio not found! (id: %lu, version: %lu)", this->matrixio_id, this->matrixio_version);
  }
}


void WishboneBus::reg_write(uint16_t add, uint16_t data) {
  return this->write(add, reinterpret_cast<uint8_t *>(&data), sizeof(uint16_t));
}

void WishboneBus::reg_read(uint16_t add, uint16_t *pdata) {
  return this->read(add, reinterpret_cast<uint8_t *>(pdata), sizeof(uint16_t));
}


void WishboneBus::read(uint16_t addr, uint8_t* data, int length){
  this->lock();

  memset( this->buffer, 0, length + sizeof(hardware_address));
  hardware_address *hw_addr = reinterpret_cast<hardware_address *>(this->buffer);
  hw_addr->reg = addr;
  hw_addr->readnwrite = 1;
  this->enable();
  this->transfer_array(this->buffer, length + sizeof(hardware_address) );
  this->disable();
  memcpy(data, &this->buffer[2], length);

  this->unlock();
}

void WishboneBus::write(uint16_t addr, const uint8_t* data, int length){
  this->lock();
  memset( this->buffer, 0, length + sizeof(hardware_address));

  hardware_address *hw_addr = reinterpret_cast<hardware_address *>(this->buffer);
  hw_addr->reg = addr;
  hw_addr->readnwrite = 0;

  memcpy(&this->buffer[2], data, length);
  this->enable();
  this->transfer_array(this->buffer, length + sizeof(hardware_address) );
  this->disable();
  this->unlock();
}

void WishboneBus::read_fpga_firmware_version(){
  uint32_t data[2];
  this->read(CONF_BASE_ADDRESS, (unsigned char*)&data, sizeof(data));
  this->matrixio_id = data[0];
  this->matrixio_version = data[1];
}


} //namespace matrixio
} //namespace esphome
