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

#pragma once

#ifdef USE_ESP_IDF

#include "audio_element.h"
#include "audio_thread.h"
#include "freertos/event_groups.h"

/**
 *  I/O Element Abstract
 */
typedef struct io_callback {
    stream_func                 cb;
    void                        *ctx;
} io_callback_t;

/**
 *  Audio Callback Abstract
 */
typedef struct audio_callback {
    event_cb_func               cb;
    void                        *ctx;
} audio_callback_t;

typedef struct audio_multi_rb {
    ringbuf_handle_t            *rb;
    int                         max_rb_num;
} audio_multi_rb_t;

typedef enum {
    IO_TYPE_RB = 1, /* I/O through ringbuffer */
    IO_TYPE_CB,     /* I/O through callback */
} io_type_t;

typedef enum {
    EVENTS_TYPE_Q = 1,  /* Events through MessageQueue */
    EVENTS_TYPE_CB,     /* Events through Callback function */
} events_type_t;


struct audio_element {
    /* Functions/RingBuffers */
    el_io_func                  open;
    ctrl_func                   seek;
    process_func                process;
    el_io_func                  close;
    el_io_func                  destroy;
    io_type_t                   read_type;
    union {
        ringbuf_handle_t        input_rb;
        io_callback_t           read_cb;
    } in;
    io_type_t                   write_type;
    union {
        ringbuf_handle_t        output_rb;
        io_callback_t           write_cb;
    } out;

    audio_multi_rb_t            multi_in;
    audio_multi_rb_t            multi_out;

    /* Properties */
    volatile bool               is_open;
    audio_element_state_t       state;

    events_type_t               events_type;
    audio_event_iface_handle_t  iface_event;
    audio_callback_t            callback_event;

    int                         buf_size;
    char                        *buf;

    char                        *tag;
    int                         task_stack;
    int                         task_prio;
    int                         task_core;
    xSemaphoreHandle            lock;
    audio_element_info_t        info;
    audio_element_info_t        *report_info;

    bool                        stack_in_ext;
    audio_thread_t              audio_thread;

    /* PrivateData */
    void                        *data;
    EventGroupHandle_t          state_event;
    int                         input_wait_time;
    int                         output_wait_time;
    int                         out_buf_size_expect;
    int                         out_rb_size;
    volatile bool               is_running;
    volatile bool               task_run;
    volatile bool               stopping;
};



#endif
