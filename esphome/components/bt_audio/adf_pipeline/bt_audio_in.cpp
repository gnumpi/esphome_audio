#include "bt_audio_in.h"

#ifdef USE_ESP_IDF

#include "../../adf_pipeline/adf_pipeline.h"
#include <a2dp_stream.h>

namespace esphome {
using namespace esp_adf;
namespace bt_audio {

static const char *const TAG = "adf_bt_audio_in";

bool ADFElementBTAudioIn::init_adf_elements_() {
  if (this->sdk_audio_elements_.size() > 0)
    return true;

  ESP_LOGI(TAG, "[4.1] Get Bluetooth stream");
  a2dp_stream_config_t a2dp_config = {
        .type = AUDIO_STREAM_READER,
        .user_callback = {0},
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0))
        .audio_hal = nullptr,
#endif
  };

  this->adf_bt_stream_reader = a2dp_stream_init(&a2dp_config);

  sdk_audio_elements_.push_back(this->adf_bt_stream_reader_);
  sdk_element_tags_.push_back("bt_audio_in");

  this->setup_bt();

  return true;
}

}
}

#endif
