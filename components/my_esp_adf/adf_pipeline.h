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

class ADFPipelineElement;
class AudioPipelineElement;


class AudioPipeline {
public:
  virtual ~AudioPipeline(){}
  
  void init();
  void reset();

  void start();
  void stop();
  void pause();
  void resume();

  void append_element( AudioPipelineElement* pipeline_element );
  int get_number_of_elements(){return pipeline_elements_.size();}
  std::vector<std::string> get_element_names();
  
  PipelineState getState(){ return state_; }
  bool request_settings(AudioPipelineSettingsRequest& request);
  void on_settings_request_failed(AudioPipelineSettingsRequest request){}

protected:
  virtual bool init_(){}
  virtual bool reset_(){}
  virtual bool start_(){}
  virtual bool stop_(){}
  virtual bool pause_(){}
  virtual bool resume_(){}
  
  void set_state_(PipelineState state);
  PipelineState state_{PipelineState::STATE_UNAVAILABLE};
  
  std::vector<AudioPipelineElement*> pipeline_elements_;
};


class ADFPipeline : public AudioPipeline {
public:
    void setup(){ init(); }
    void watch(){this->watch_();}

    ADFPipelineElement* get_first_component(){ return this->pipeline_elements_[0]; }
    ADFPipelineElement* get_last_component (){ return this->pipeline_elements_.back(); };

protected:
    bool init_()   override;
    bool reset_()  override;
    bool start_()  override;
    bool stop_()   override;
    bool pause_()  override;
    bool resume_() override;

    void watch_();
    void forward_event_to_pipeline_elements_(audio_event_iface_msg_t &msg);

    bool build_adf_pipeline_();
    bool terminate_pipeline_();
    void deinit_all_();
    
    audio_pipeline_handle_t adf_pipeline_{};
    audio_event_iface_handle_t adf_pipeline_event_{};
    audio_element_handle_t adf_last_element_in_pipeline_{};
};

/*
An ESPHome Component for managing an ADFPipeline
*/
class ADFPipelineComponent : public Component {
public:
  virtual void append_own_elements(){}
  void add_element_to_pipeline( ADFPipelineElement* elements );

  void setup() override {}
  void dump_config() override;
  void loop() override {this->pipeline.watch();}

protected:
  virtual void pipeline_event_handler(audio_event_iface_msg_t &msg){}
  ADFPipeline pipeline; 
};



}
}

#endif