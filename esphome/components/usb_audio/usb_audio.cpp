#include "usb_audio.h"

#ifdef USE_ESP32_USB_STREAM

#include "esphome/core/log.h"
#include <inttypes.h>

namespace esphome {
namespace usb_audio {

static const char *const TAG = "usb_audio";

static void stream_state_changed_cb(usb_stream_state_t event, void* param )
{
  USBAudioComponent *comp = reinterpret_cast<USBAudioComponent*>(param);
  comp->on_stream_state_changed(event);
}

void USBAudioComponent::setup_usb(){
    uac_config_t uac_config = {
        .spk_ch_num = UAC_CH_ANY,
        .spk_bit_resolution = UAC_BITS_ANY,
        .spk_samples_frequence = UAC_FREQUENCY_ANY,
        .spk_buf_size = 25600,// 96000, // size of speaker send buffer, should be a multiple of spk_ep_mps

        .mic_interface = 0,              /*!< (optional) microphone stream interface number, set 0 if not use */
        .mic_ep_addr = 0,                /*!< (optional) microphone interface endpoint address */
        .mic_ep_mps = 0,                 /*!< (optional) microphone interface endpoint mps */
                                      /*!< (optional) speaker stream interface number, set 0 if not use */
        //spk_ep_addr;                /*!< (optional) speaker interface endpoint address */
        .spk_ep_mps = 512,   //max 512              /*!< (optional) speaker interface endpoint mps */
        //ac_interface;               /*!< (optional) audio control interface number, set 0 if not use */
        //mic_fu_id;                  /*!< (optional) microphone feature unit id, set 0 if not use */
        //spk_fu_id;                  /*!< (optional) speaker feature unit id, set 0 if not use */

        .flags = 0 //FLAG_UAC_SPK_SUSPEND_AFTER_START

    };
    ESP_ERROR_CHECK(uac_streaming_config(&uac_config));
    ESP_ERROR_CHECK(usb_streaming_state_register(&stream_state_changed_cb, this));
}

void USBAudioComponent::start_streaming(){
    ESP_ERROR_CHECK(usb_streaming_start());
}

void USBAudioComponent::stop_streaming(){
    ESP_ERROR_CHECK(usb_streaming_stop());
}

void USBAudioComponent::read_frame_size_list(){
   size_t frame_size = 0;
   size_t frame_index = 0;
   uac_frame_size_list_get(STREAM_UAC_SPK, NULL, &frame_size, &frame_index);
   if (frame_size) {
        ESP_LOGI(TAG, "UAC SPK: get frame list size = %u, current = %u", frame_size, frame_index);
        uac_frame_size_t *spk_frame_list = (uac_frame_size_t *)malloc(frame_size * sizeof(uac_frame_size_t));
        uac_frame_size_list_get(STREAM_UAC_SPK, spk_frame_list, NULL, NULL);
        for (size_t i = 0; i < frame_size; i++) {
            ESP_LOGI(TAG, "\t [%u] ch_num = %u, bit_resolution = %u, samples_frequence = %"PRIu32 ", samples_frequence_min = %"PRIu32 ", samples_frequence_max = %"PRIu32,
                      i, spk_frame_list[i].ch_num, spk_frame_list[i].bit_resolution, spk_frame_list[i].samples_frequence,
                      spk_frame_list[i].samples_frequence_min, spk_frame_list[i].samples_frequence_max);
        }
        ESP_LOGI(TAG, "UAC SPK: use frame[%u] ch_num = %u, bit_resolution = %u, samples_frequence = %"PRIu32,
                  frame_index, spk_frame_list[frame_index].ch_num, spk_frame_list[frame_index].bit_resolution, spk_frame_list[frame_index].samples_frequence);
        this->channels_ = spk_frame_list[frame_index].ch_num;
        this->bits_per_sample_ = spk_frame_list[frame_index].bit_resolution;
        this->sample_rate_ = spk_frame_list[frame_index].samples_frequence;
        usb_streaming_control(STREAM_UAC_SPK, CTRL_UAC_VOLUME, (void *)45);
        free(spk_frame_list);
   } else {
        ESP_LOGW(TAG, "UAC SPK: get frame list size = %u", frame_size);
   }
}


void USBAudioComponent::on_stream_state_changed(usb_stream_state_t event){
    switch (event) {
    case STREAM_CONNECTED:
        ESP_LOGI(TAG, "Device connected");
        this->read_frame_size_list();
        this->on_connected();
        break;
    case STREAM_DISCONNECTED:
         ESP_LOGI(TAG, "Device disconnected");
        this->on_disconnected();
        break;
    default:
        ESP_LOGE(TAG, "Unknown event");
        break;
    }
}


}
}

#endif
