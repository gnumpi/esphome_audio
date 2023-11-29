#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/light/addressable_light.h"

#include "esphome/components/matrixio/wishbone.h"

namespace esphome {
namespace matrixio {

const uint32_t EVERLOOP_BASE_ADDRESS = 0x3000;
const uint32_t NUMBER_OF_LEDS = 18;

class Everloop : public light::AddressableLight, public matrixio::WishboneDevice {
public:
    Everloop() : matrixio::WishboneDevice(EVERLOOP_BASE_ADDRESS), num_leds_(NUMBER_OF_LEDS) {}

    int32_t size() const override { return this->num_leds_; }

    void setup();
    void dump_config();

    light::LightTraits get_traits() override {
        auto traits = light::LightTraits();
        traits.set_supported_color_modes({light::ColorMode::RGB_WHITE});
        return traits;
    }

    void write_state(light::LightState *state) override {
        if (this->is_failed())
            return;
        this->wb_write(this->buf_, this->buffer_size_);
    }

    void clear_effect_data() override {
        for (int i = 0; i < this->size(); i++)
            this->effect_data_[i] = 0;
    }

protected:
    light::ESPColorView get_view_internal(int32_t index) const override;

private:
    size_t buffer_size_{};
    uint8_t *effect_data_{nullptr};
    uint8_t *buf_{nullptr};
    int32_t num_leds_;
};

} //namespace matrixio
} //namespace esphome
