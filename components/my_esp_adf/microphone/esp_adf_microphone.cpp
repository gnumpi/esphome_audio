#include "esp_adf_microphone.h"

#ifdef USE_ESP_IDF

#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace esp_adf {

void ADFMicrophone::setup(){

}

void ADFMicrophone::dump_config(){
   
}

void ADFMicrophone::loop(){
    pipeline.loop();
}

void ADFMicrophone::start(){
    pipeline.start();
}

void ADFMicrophone::stop(){
    pipeline.stop();
}

size_t ADFMicrophone::read(int16_t *buf, size_t len){
    return pcm_stream_.stream_read( (char*) buf, len);
}

}
}
#endif