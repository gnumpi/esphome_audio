/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/i2s.h"
#include "esp_log.h"
#include "esp_err.h"

#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "i2s_stream_mod.h"
#include "esp_alc.h"
#include "board_pins_config.h"
#include "audio_idf_version.h"

static const char *TAG = "I2S_STREAM";


#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 2, 0)
#define SOC_I2S_SUPPORTS_ADC_DAC 1
#elif (SOC_DAC_SUPPORTED)
#if CONFIG_IDF_TARGET_ESP32
#define SOC_I2S_SUPPORTS_ADC_DAC 1
#endif
#endif

#ifdef SOC_I2S_SUPPORTS_ADC_DAC
static esp_err_t i2s_mono_fix(int bits, uint8_t *sbuff, uint32_t len)
{
    if (bits == 16) {
        int16_t *temp_buf = (int16_t *)sbuff;
        int16_t temp_box;
        int k = len >> 1;
        for (int i = 0; i < k; i += 2) {
            temp_box = temp_buf[i];
            temp_buf[i] = temp_buf[i + 1];
            temp_buf[i + 1] = temp_box;
        }
    } else if (bits == 32) {
        int32_t *temp_buf = (int32_t *)sbuff;
        int32_t temp_box;
        int k = len >> 2;
        for (int i = 0; i < k; i += 4) {
            temp_box = temp_buf[i];
            temp_buf[i] = temp_buf[i + 1];
            temp_buf[i + 1] = temp_box;
        }
    } else {
        ESP_LOGE(TAG, "%s %dbits is not supported", __func__, bits);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * @brief Scale data to 16bit/32bit for I2S DMA output.
 *        DAC can only output 8bit data value.
 *        I2S DMA will still send 16bit or 32bit data, the highest 8bit contains DAC data.
 */
static int i2s_dac_data_scale(int bits, uint8_t *sBuff, uint32_t len)
{
    if (bits == 16) {
        short *buf16 = (short *)sBuff;
        int k = len >> 1;
        for (int i = 0; i < k; i++) {
            buf16[i] &= 0xff00;
            buf16[i] += 0x8000;//turn signed value into unsigned, expand negative value into positive range
        }
    } else if (bits == 32) {
        int *buf32 = (int *)sBuff;
        int k = len >> 2;
        for (int i = 0; i < k; i++) {
            buf32[i] &= 0xff000000;
            buf32[i] += 0x80000000;//turn signed value into unsigned
        }
    } else {
        ESP_LOGE(TAG, "in %s %dbits is not supported", __func__, bits);
        return -1;
    }

    return 0;
}
#endif

static int i2s_stream_clear_dma_buffer(audio_element_handle_t self)
{
    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(self);
    int index = i2s->config.i2s_config.dma_buf_count;
    uint8_t *buf = audio_calloc(1, i2s->config.i2s_config.dma_buf_len * 4);
    AUDIO_MEM_CHECK(TAG, buf, return ESP_ERR_NO_MEM);
#if SOC_I2S_SUPPORTS_ADC_DAC
    if ((i2s->config.i2s_config.mode & I2S_MODE_DAC_BUILT_IN) != 0) {
        memset(buf, 0x80, i2s->config.i2s_config.dma_buf_len * 4);
    }
#endif
    while (index--) {
        audio_element_output(self, (char *)buf, i2s->config.i2s_config.dma_buf_len * 4);
    }
    if (buf) {
        audio_free(buf);
    }
    return ESP_OK;
}

static esp_err_t _i2s_set_clk(i2s_port_t i2s_num, uint32_t rate, uint32_t bits_cfg, int ch)
{
    i2s_channel_t channel;
    if (ch == 1) {
        channel = I2S_CHANNEL_MONO;
    } else if (ch == 2) {
        channel = I2S_CHANNEL_STEREO;
    } else {
        return ESP_FAIL;
    }
    return i2s_set_clk(i2s_num, rate, bits_cfg, channel);
}

static esp_err_t _i2s_open(audio_element_handle_t self)
{
    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(self);
    ESP_LOGD(TAG, "_i2s_open %d", (int)i2s->config.i2s_port);
    if (i2s->is_open) {
        return ESP_OK;
    }

    /*
    if (i2s->type == AUDIO_STREAM_WRITER) {
        audio_element_set_input_timeout(self, 10 / portTICK_RATE_MS);
        ESP_LOGI(TAG, "AUDIO_STREAM_WRITER");
    }
    */
    i2s->is_open = true;
    if (i2s->use_alc) {
        i2s->volume_handle = alc_volume_setup_open();
        if (i2s->volume_handle == NULL) {
            ESP_LOGE(TAG, "i2s create the handle for setting volume failed, in line(%d)", __LINE__);
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

static esp_err_t _i2s_destroy(audio_element_handle_t self)
{
    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(self);
    if (i2s->uninstall_drv) {
        i2s_driver_uninstall(i2s->config.i2s_port);
    }
    audio_free(i2s);
    return ESP_OK;
}

static esp_err_t _i2s_close(audio_element_handle_t self)
{
    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(self);
    esp_err_t ret = i2s_stream_clear_dma_buffer(self);
    if (ret != ESP_OK) {
        return ret;
    }
    i2s->is_open = false;
    if (AEL_STATE_PAUSED != audio_element_get_state(self)) {
        audio_element_report_pos(self);
        audio_element_set_byte_pos(self, 0);
    }
    if (i2s->use_alc) {
        if (i2s->volume_handle != NULL) {
            alc_volume_setup_close(i2s->volume_handle);
        }
    }
    return ESP_OK;
}

static int _i2s_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(self);
    size_t bytes_read = 0;
    i2s_read(i2s->config.i2s_port, buffer, len, &bytes_read, ticks_to_wait);
    audio_element_info_t info;
    audio_element_getinfo(self, &info);

    if (bytes_read > 0) {
#ifdef CONFIG_IDF_TARGET_ESP32
        if (info.channels == 1) {
            i2s_mono_fix(info.bits, (uint8_t *)buffer, bytes_read);
        }
#endif
    }

    /*
    int REAL_BYTES_X_SAMPLE = 4;

    //i2s_read(I2S_NUM_0, (void*)dataOrig, buffSizeOrig, &bytesRead, portMAX_DELAY);
    uint16_t samplesRead = bytes_read / REAL_BYTES_X_SAMPLE;
    for (int i = 0; i < samplesRead; i++) {
        int byteIndex = i * REAL_BYTES_X_SAMPLE;
        int32_t value = ((int32_t*)(buffer + byteIndex))[0]>>8;
    }
    */
    return bytes_read;
}

static int _i2s_write(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(self);
    len = len >> 2 << 2;
    size_t bytes_written = 0;
    audio_element_info_t info;
    audio_element_getinfo(self, &info);
    if (len > 0) {
#ifdef CONFIG_IDF_TARGET_ESP32
        if (info.channels == 1) {
            i2s_mono_fix(info.bits, (uint8_t *)buffer, len);
        }
#endif
#if SOC_I2S_SUPPORTS_ADC_DAC
        if ((i2s->config.i2s_config.mode & I2S_MODE_DAC_BUILT_IN) != 0) {
            i2s_dac_data_scale(info.bits, (uint8_t *)buffer, len);
        }
#endif
    }

    if (i2s->config.need_expand && (i2s->config.i2s_config.bits_per_sample != i2s->config.expand_src_bits)) {
        i2s_write_expand(i2s->config.i2s_port,
                         buffer,
                         len,
                         i2s->config.expand_src_bits,
                         i2s->config.i2s_config.bits_per_sample,
                         &bytes_written,
                         ticks_to_wait);
    } else {
        i2s_write(i2s->config.i2s_port, buffer, len, &bytes_written, ticks_to_wait);
    }

    return bytes_written;
}

static int _i2s_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    int r_size = audio_element_input(self, in_buffer, in_len);
    int w_size = 0;
    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(self);
    audio_element_info_t i2s_info = {0};
    if (r_size == AEL_IO_TIMEOUT) {
#if SOC_I2S_SUPPORTS_ADC_DAC
        if ((i2s->config.i2s_config.mode & I2S_MODE_DAC_BUILT_IN) != 0) {
            memset(in_buffer, 0x80, in_len);
        } else
#endif
        {
            if( i2s->finish_on_timeout ){
                ESP_LOGI(TAG, "Sending finish status after time out occurred." );
                audio_element_report_status(self, AEL_STATUS_STATE_FINISHED);
                return 0;
            }
            else {
                memset(in_buffer, 0x00, in_len);
            }
        }
        r_size = in_len;
        audio_element_multi_output(self, in_buffer, r_size, 0);
        w_size = audio_element_output(self, in_buffer, r_size);
    } else if (r_size > 0) {
        if (i2s->use_alc) {
            audio_element_getinfo(self, &i2s_info);
            alc_volume_setup_process(in_buffer, r_size, i2s_info.channels, i2s->volume_handle, i2s->volume);
        }
        audio_element_multi_output(self, in_buffer, r_size, 0);
        w_size = audio_element_output(self, in_buffer, r_size);
        audio_element_update_byte_pos(self, w_size);
    } else {
        esp_err_t ret = i2s_stream_clear_dma_buffer(self);
        if (ret != ESP_OK) {
            return ret;
        }
        w_size = r_size;
    }
    return w_size;
}

esp_err_t i2s_stream_set_clk(audio_element_handle_t i2s_stream, int rate, int bits, int ch)
{
    esp_err_t err = ESP_OK;
    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(i2s_stream);
    audio_element_state_t state = audio_element_get_state(i2s_stream);
    if (state == AEL_STATE_RUNNING) {
        audio_element_pause(i2s_stream);
    }
    audio_element_set_music_info(i2s_stream, rate, ch, bits);

    if (_i2s_set_clk(i2s->config.i2s_port, rate, bits, ch) == ESP_FAIL) {
        ESP_LOGE(TAG, "i2s_set_clk failed, type = %d,port:%d", i2s->config.type, i2s->config.i2s_port);
        err = ESP_FAIL;
    }
    if (state == AEL_STATE_RUNNING) {
        audio_element_resume(i2s_stream, 0, 0);
    }
    return err;
}

int i2s_alc_volume_set(audio_element_handle_t i2s_stream, int volume)
{
    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(i2s_stream);
    if (i2s->use_alc) {
        i2s->volume = volume;
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "The ALC don't be used. It can not be set.");
        return ESP_FAIL;
    }
}

int i2s_alc_volume_get(audio_element_handle_t i2s_stream, int *volume)
{
    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(i2s_stream);
    if (i2s->use_alc) {
        *volume = i2s->volume;
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "The ALC don't be used");
        return ESP_FAIL;
    }
}

audio_element_handle_t i2s_stream_init(i2s_stream_cfg_t *config)
{
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    audio_element_handle_t el;
    cfg.open = _i2s_open;
    cfg.close = _i2s_close;
    cfg.process = _i2s_process;
    cfg.destroy = _i2s_destroy;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.stack_in_ext = config->stack_in_ext;
    cfg.out_rb_size = config->out_rb_size;
    cfg.multi_out_rb_num = config->multi_out_num;
    cfg.tag = "iis";
    cfg.buffer_len = I2S_STREAM_BUF_SIZE;

    i2s_stream_t *i2s = audio_calloc(1, sizeof(i2s_stream_t));
    AUDIO_MEM_CHECK(TAG, i2s, return NULL);
    memcpy(&i2s->config, config, sizeof(i2s_stream_cfg_t));

    i2s->type = config->type;
    i2s->use_alc = config->use_alc;
    i2s->volume = config->volume;
    i2s->uninstall_drv = config->uninstall_drv;
    i2s->finish_on_timeout = config->finish_on_timeout;

    if (config->type == AUDIO_STREAM_READER) {
        cfg.read = _i2s_read;
    } else if (config->type == AUDIO_STREAM_WRITER) {
        cfg.write = _i2s_write;
    }


    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, {
        audio_free(i2s);
        return NULL;
    });
    audio_element_setdata(el, i2s);

    audio_element_set_music_info(el, config->i2s_config.sample_rate,
                                 config->i2s_config.channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT ? 2 : 1,
                                 config->i2s_config.bits_per_sample);
#if SOC_I2S_SUPPORTS_ADC_DAC
    if ((config->i2s_config.mode & I2S_MODE_DAC_BUILT_IN) != 0) {
        i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    }
#endif
    //i2s_mclk_gpio_select(i2s->config.i2s_port, GPIO_NUM_0);

    return el;
}

esp_err_t i2s_stream_sync_delay(audio_element_handle_t i2s_stream, int delay_ms)
{
    char *in_buffer = NULL;

    audio_element_info_t info;
    audio_element_getinfo(i2s_stream, &info);

    if (delay_ms < 0) {
        uint32_t delay_size = (~delay_ms + 1) * ((uint32_t)(info.sample_rates * info.channels * info.bits / 8) / 1000);
        in_buffer = (char *)audio_malloc(delay_size);
        AUDIO_MEM_CHECK(TAG, in_buffer, return ESP_FAIL);
#if SOC_I2S_SUPPORTS_ADC_DAC
        i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(i2s_stream);
        if ((i2s->config.i2s_config.mode & I2S_MODE_DAC_BUILT_IN) != 0) {
            memset(in_buffer, 0x80, delay_size);
        } else
#endif
        {
            memset(in_buffer, 0x00, delay_size);
        }
        ringbuf_handle_t input_rb = audio_element_get_input_ringbuf(i2s_stream);
        if (input_rb) {
            rb_write(input_rb, in_buffer, delay_size, 0);
        }
        audio_free(in_buffer);
    } else if (delay_ms > 0) {
        uint32_t drop_size = delay_ms * ((uint32_t)(info.sample_rates * info.channels * info.bits / 8) / 1000);
        in_buffer = (char *)audio_malloc(drop_size);
        AUDIO_MEM_CHECK(TAG, in_buffer, return ESP_FAIL);
        uint32_t r_size = audio_element_input(i2s_stream, in_buffer, drop_size);
        audio_free(in_buffer);

        if (r_size > 0) {
            audio_element_update_byte_pos(i2s_stream, r_size);
        } else {
            ESP_LOGW(TAG, "Can't get enough data to drop.");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}
