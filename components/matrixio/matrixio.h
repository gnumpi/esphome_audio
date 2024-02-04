#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"

namespace esphome {
namespace matrixio {

const uint16_t AUDIO_OUT_BASE_ADDRESS = 0x6000;
const uint16_t MAX_VOLUME_VALUE = 25;
const uint32_t FPGA_FIFO_SIZE = 4096;
const uint32_t MAX_WRITE_LENGTH = 1024;
//const uint32_t kDACFrequency = 4000000;

static const size_t BUFFER_SIZE = 1024;

const uint32_t PCM_SAMPLING_FREQUENCIES[][2] = {
    {8000, 975},  {16000, 492}, {32000, 245}, {44100, 177},
    {48000, 163}, {88200, 88},  {96000, 81},  {0, 0}};

enum MuteStatus : uint16_t {
  kMute = 0x0001,
  kUnMute = 0x0000
};

enum OutputSelector : uint16_t {
  kHeadPhone = 0x0001,
  kSpeaker = 0x0000
};

// (sampling_rate, pdm_decimation, gain)
static const uint32_t MIC_SAMPLING_FREQUENCIES[][3] = {
    {8000, 374, 0},  {12000, 249, 2}, {16000, 186, 3}, {22050, 135, 5},
    {24000, 124, 5}, {32000, 92, 6},  {44100, 67, 7},  {48000, 61, 7},
    {96000, 30, 9},  {0, 0, 0}};

const uint32_t MICROPHONE_BASE_ADDRESS = 0x2000;
const uint16_t MICROPHONE_ARRAY_IRQ = 5;
const uint16_t NUMBER_OF_FIR_TAPS = 128;
const uint16_t MICROPHONE_CHANNELS = 8;


}
}