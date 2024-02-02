#pragma once

#include <cstring>
#include <vector>
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF 

#include <audio_element.h>
#include <audio_pipeline.h>

#include "adf_audio_element.h"

namespace esphome {
namespace esp_adf {

enum PipelineState : uint8_t {
  STATE_UNAVAILABLE = 0,
  STATE_STOPPED,
  STATE_RUNNING,
  STATE_PAUSED,
};

class AudioPipeline {
public:
  void init();
  void reset();

  void start();
  void stop();
  void pause();
  void resume();
  
  PipelineState getState(){ return state_; }

protected:
  virtual bool init_()  {return true;}
  virtual bool reset_() {return true;}
  virtual bool start_() {return true;}
  virtual bool stop_()  {return true;}
  virtual bool pause_() {return true;}
  virtual bool resume_(){return true;}
  
  virtual void set_state_(PipelineState state){ state_ = state;}
  
  PipelineState state_{PipelineState::STATE_UNAVAILABLE};
};


class ADFPipeline : public AudioPipeline {
public:
  virtual ~ADFPipeline(){}
  
  void loop(){this->watch_();}

  void append_element( ADFPipelineElement* element );
  int get_number_of_elements(){return pipeline_elements_.size();}
  std::vector<std::string> get_element_names();
  
  bool request_settings(AudioPipelineSettingsRequest& request);
  void on_settings_request_failed(AudioPipelineSettingsRequest request){}

protected:
    bool init_()   override;
    bool reset_()  override;
    bool start_()  override;
    bool stop_()   override;
    bool pause_()  override;
    bool resume_() override;
    
    void set_state_(PipelineState state) override;
    
    void watch_();
    void forward_event_to_pipeline_elements_(audio_event_iface_msg_t &msg);

    bool build_adf_pipeline_();
    bool terminate_pipeline_();
    void deinit_all_();
    
    audio_pipeline_handle_t adf_pipeline_{};
    audio_event_iface_handle_t adf_pipeline_event_{};
    audio_element_handle_t adf_last_element_in_pipeline_{};
    std::vector<ADFPipelineElement*> pipeline_elements_;
};

/*
An ESPHome Component for managing an ADFPipeline
*/
class ADFPipelineComponent : public Component {
public:
  virtual void append_own_elements(){}
  void add_element_to_pipeline( ADFPipelineElement* element ){
    pipeline.append_element(element);
  }

  void setup() override {}
  void dump_config() override;
  void loop() override {pipeline.loop();}

protected:
  virtual void pipeline_event_handler(audio_event_iface_msg_t &msg){}
  ADFPipeline pipeline; 
};



}
}

#endif