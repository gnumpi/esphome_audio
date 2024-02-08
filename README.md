# ESPHome - Audio Components

## ADF-Pipeline

The purpose of this ESPHome component is to offer a wrapper framework that enables access to the elements from the [Espressif Audio Development Framework (ADF)](https://github.com/espressif/esp-adf) within the ESPHome environment.



### The following elements are available for now:

**Pipeline elements:**

* *I2SReader*
* *PCMSource*
* *HTTPStreamReaderAndDecoder*
* *I2SWriter*
* *PCMSink*

**ESPHome components:**

new platform: 'adf_pipeline'

* The included extension for the 'i2s_audio' provides i2s configurations as pipeline elements.


Pipeline controller:

*  *ADFMicrophone*: 'esphome.microphone' platform implementation
*  *ADFSpeaker*: 'esphome.speaker' platform implementation
*  *ADFMediaPlayer*: 'esphome.media_player' platform implementation

Pipeline controllers are responsible for constructing an audio pipeline and provide methods for controlling the pipeline (e.g. start, stop, pause ).



### Configuration
```yaml
external_components:
  - source:
      type: git
      url: https://github.com/gnumpi/esphome_audio
      ref: main
    components: [ adf_pipeline, i2s_audio ]


# define the i2s controller and their pins as before
i2s_audio:
  - id: i2s_in
    i2s_lrclk_pin: GPIO5
    i2s_bclk_pin: GPIO6
  - id: i2s_out
    i2s_lrclk_pin: GPIO46
    i2s_bclk_pin: GPIO9

# expose the i2s components as pipeline elements
adf_pipeline:
    # create a I2SWriter pipeline element
    # using the i2s_out configuration
  - platform: i2s_audio
    type: sink
    id: adf_i2s_out
    i2s_audio_id: i2s_out
    i2s_dout_pin: GPIO10

    # create a I2SReader pipeline element
    # using the i2s_in configuration
  - platform: i2s_audio
    type: source
    id: adf_i2s_in
    i2s_audio_id: i2s_in
    i2s_din_pin: GPIO4


# create a new microphone component which
# is implemented as an adf_pipeline
microphone:
  - platform: adf_pipeline
    id: adf_microphone
    #define the pipeline
    pipeline:
        # take the I2SReader pipeline element
        # as the first element (SOURCE)
      - adf_i2s_in
        # The microphone implementation implicitly
        # creates a PCMSink pipeline_element which
        # provides the raw audio data to other esphome components
        # using the microphone platform interface.
        # It is added as the last element (sink) for the pipeline.
      - self

# create a new speaker component which
# is implemented as an adf_pipeline
speaker:
  - platform: adf_pipeline
    id: adf_speaker
    #define the pipeline
    pipeline:
        # The speaker implementation implicitly
        # creates a PCMSource pipeline_element to
        # which other ESPHome components can write to
        # using the speaker platform interface.
        # It is added as the first element, as a source to
        # the pipeline
      - self
        # add the I2SWriter pipeline element as a sink.
      - adf_i2s_out


# create a new media_player component which
# is implemented as an adf_pipeline
media_player:
  - platform: adf_pipeline
    id: adf_media_player
    name: s3-dev_media_player
    internal: false
    pipeline:
        # The media_player implementation implicitly
        # creates a HTTPStreamReaderAndDecoder pipeline_element,
        # which can be controlled using the media_player interface.
        # It is added as the first element, as source to
        # the pipeline
      - self
        # add the I2SWriter pipeline element as a sink.
      - adf_i2s_out

voice_assistant:
  microphone: adf_microphone
  #if a media player is defined it should be used instead of the speaker
  media_player: adf_media_player
```

## Notes:
* using the same element in two pipelines (e.g. using adf_i2s_out in the speaker and the media_player) is not supported yet
* using the adf_pipeline component disables the verification of server certificates by setting the idf-sdk option "CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY". This is quick and dirty hack for allowing streaming from internet radio stations, be aware of the potential security issue.
