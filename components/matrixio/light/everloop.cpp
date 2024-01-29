#include "everloop.h"

namespace esphome {
namespace matrixio {

static const char *const TAG = "matrixio_everloop";
  Everloop::Everloop() : matrixio::WishboneDevice(EVERLOOP_BASE_ADDRESS), num_leds_(NUMBER_OF_LEDS){
} 

void Everloop::setup() {
  ExternalRAMAllocator<uint8_t> allocator(ExternalRAMAllocator<uint8_t>::ALLOW_FAILURE);
  this->buffer_size_ = this->size() * 4;
  this->buf_ = allocator.allocate(this->buffer_size_);
  if (this->buf_ == nullptr) {
      esph_log_e(TAG, "Failed to allocate buffer of size %u", this->buffer_size_);
      this->mark_failed();
      return;
  }
  this->effect_data_ = allocator.allocate(this->size());
  if (this->effect_data_ == nullptr) {
    esph_log_e(TAG, "Failed to allocate effect data of size %u", this->num_leds_);
    this->mark_failed();
    return;
  }
  memset(this->buf_, 0, this->buffer_size_);  
}

float Everloop::get_setup_priority() const { return setup_priority::HARDWARE; }

void Everloop::dump_config() {
  esph_log_config(TAG, "Matrixio Everloop:");
  esph_log_config(TAG, "  LEDs: %d", this->num_leds_);
}

light::ESPColorView Everloop::get_view_internal(int32_t index) const {
        size_t pos = index * 4;
        return {this->buf_ + pos + 0, 
                this->buf_ + pos + 1, 
                this->buf_ + pos + 2, 
                this->buf_ + pos + 3,
                this->effect_data_ + index, 
                &this->correction_ 
              };
    }

}
}
