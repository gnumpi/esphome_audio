# ESPHome - Audio Components
**Min-Version: ESPHome-2024.4**

[![ESPHome-target](https://github.com/gnumpi/esphome_audio/actions/workflows/tox-target.yml/badge.svg)](https://github.com/gnumpi/esphome_audio/actions/workflows/tox-target.yml)
[![ESPHome-latest](https://github.com/gnumpi/esphome_audio/actions/workflows/tox-latest.yml/badge.svg)](https://github.com/gnumpi/esphome_audio/actions/workflows/tox-latest.yml)

The purpose of these ESPHome components is to offer a wrapper framework that enables access to the elements from the [Espressif Audio Development Framework (ADF)](https://github.com/espressif/esp-adf) within the ESPHome environment.

## Custom Components

### i2s_audio

This customized version of *i2s_audio* offers several enhancements:

- **Introduces `I2SReader` and `I2SWriter` classes:** These serve as the base for the *i2s-microphone* and *i2s-speaker*, respectively.
- **Separates I2S settings into a distinct class:** This enhances reusability across other components, such as elements within the *adf-pipeline*.
- **Includes an implementation for the *adf_pipeline* platform:** This integrates the I2S-Reader and I2S-Writer as pipeline elements.
- **Supports duplex-mode operation:** Allows simultaneous reading and writing access to an I2S port.
- **Enhances functionality with ADCs and DACs:** Ability to set up and control external DACs and ADCs via the I2C protocol.

### adf_pipeline

This custom component includes:

- **Integration support for *media_player* within the IDF framework**
- **Flexible setup combining various audio transports:** Allows combination of *microphone*, *speaker*, and *media_player* with different audio transports such as I2S, http, Bluetooth (work in progress), USB (work in progress), and custom implementations (e.g., Wishbone via SPI). See more [here](https://github.com/gnumpi/esphome_matrixio).
- **Optional resampling feature:** Facilitates the resampling of the audio stream to accommodate different output requirements.

### Configurations

**General**
```yaml
external_components:
  - source:
      type: git
      url: https://github.com/gnumpi/esphome_audio
      ref: main
    components: [ adf_pipeline, i2s_audio ]

esp32:
  board: esp32-s3-devkitc-1
  flash_size: 16MB
  framework:
    type: esp-idf
    sdkconfig_options:
      # need to set a s3 compatible board for the adf-sdk to compile
      # board specific code is not used though
      CONFIG_ESP32_S3_BOX_BOARD: "y"
```
#### I2S-Settings:
- **i2s_audio_id** (*Optional*, :ref:`config-id`): The ID of the :ref:`I²S Audio <i2s_audio>` you wish to use for this component.
- **channel** (*Optional*, enum): For an I2S-Reader, the I2S channel to read from. One of ``right``, ``left`` and ``right_left``. By setting it to ``right_left``, both channels are read. For an I2S-Writer, it decides whether the PCM stream is interpreted as a mono (``right``) or stereo (``right_left``) stream. In mono mode, the PCM stream is written to both I2S channels. Defaults to ``right_left``.
- **sample_rate** (*Optional*, positive integer): I2S sample rate. Defaults to ``16000``.
- **bits_per_sample** (*Optional*, enum): The bit depth of the audio samples.
  One of ``16bit`` or ``32bit``. Defaults to ``16bit``.
- **use_apll** (*Optional*, boolean): I2S using APLL as main I2S clock, enable it to get accurate clock. Defaults to ``false``.
- **fixed_settings** (*Optional*, boolean): I2S-settings are not allowed to be changed dynamically if set to true. Defaults to ``false``.



For configuration examples not utilizing the *adf_pipeline*, please refer to the following YAML files:

- **esp32-s3-N16R8-spk.yaml**: Uses a dedicated I2S port for both microphone and speaker.
- **m5stack-atom-echo-spk.yaml**: Shared I2S port with exclusive access.
- **m5stack-core-s3-spk.yaml**: includes the use of external DACs and ADCs

#### ADF-MediaPlayer announcement configurations:
When playing an audio stream the stream is additionally started once silently in the preparation phase in order to detect the audio format and set the pipeline components accordingly. In order to safe some time this detection phase can be skipped for announcements which always have the same audio settings:

```yaml
media_player:
  - platform: adf_pipeline
    id: adf_media_player
    name: s3-dev_media_player
    internal: false
    keep_pipeline_alive: false
    announcement_audio:
      sample_rate: 16000
      bits_per_sample: 16
      num_channels: 1
    pipeline:
      - self
      - resampler
      - adf_i2s_out

```

#### ADF-Pipeline configurations:

**Dedicated I2S-Port for Microphone and Speaker/Media Player:**

Both input and output components have exclusive access to their assigned I2S-Port, allowing I2S settings to be configured independently of each other. ADF-Pipeline elements can remain initialized even when the pipeline is idle, with **keep_pipeline_alive** set to *true*.


Example config (see also: esp32-s3-N16R8-adf.yaml)
```yaml
# define the i2s controller and their pins as before
i2s_audio:
  - id: i2s_in
    i2s_lrclk_pin: GPIO5
    i2s_bclk_pin: GPIO6
  - id: i2s_out
    i2s_lrclk_pin: GPIO46
    i2s_bclk_pin: GPIO9

adf_pipeline:
  - platform: i2s_audio
    type: audio_out
    id: adf_i2s_out
    i2s_audio_id: i2s_out
    i2s_dout_pin: GPIO10

  - platform: i2s_audio
    type: audio_in
    id: adf_i2s_in
    i2s_audio_id: i2s_in
    i2s_din_pin: GPIO4
    pdm: false
    channel: left
    sample_rate: 16000
    bits_per_sample: 32bit


microphone:
  - platform: adf_pipeline
    id: adf_microphone
    gain_log2: 3
    keep_pipeline_alive: true
    pipeline:
      - adf_i2s_in
      - self

media_player:
  - platform: adf_pipeline
    id: adf_media_player
    name: s3-dev_media_player
    keep_pipeline_alive: true
    internal: false
    pipeline:
      - self
      - adf_i2s_out
```

**Shared I2S-Port with Exclusive Access**

Either the `I2SWriter` or the `I2SReader` can be active at a time and must release the I2S-Port when not in use. Therefore, **keep_pipeline_alive** should be set to *false*. Since each component loads its own I2S driver, I2S settings can be configured independently for each component.

Example config (see also: m5stack-atom-echo-adf.yaml)
```yaml
i2s_audio:
  - id: i2s_shared
    i2s_lrclk_pin: GPIO33
    i2s_bclk_pin: GPIO19
    access_mode: exclusive

adf_pipeline:
  - platform: i2s_audio
    type: audio_out
    id: adf_i2s_out
    i2s_audio_id: i2s_shared
    i2s_dout_pin: GPIO22
    fixed_settings: false

  - platform: i2s_audio
    type: audio_in
    id: adf_i2s_in
    i2s_audio_id: i2s_shared
    i2s_din_pin: GPIO23
    pdm: true
    bits_per_sample: 32bit
    channel: right
    fixed_settings: true
```


**Shared I2S Port with Duplex Access**

The I2S driver is installed only once and configured with shared settings that are used by both the I2S Reader and the I2S Writer. It's important to ensure that these settings are compatible for both components. To prevent interference from other components, such as the *media_player*, which might try to modify the I2S configuration, set **fixed_settings** to ``true``.

For enhanced compatibility and to support dynamic audio configurations, integrate a *resampler* into the ADF-pipeline. This will help in adjusting audio sample rates or formats dynamically, facilitating smooth operation across different audio processing components.

Example config (see also: m5stack-core-s3-adf.yaml)
```yaml
i2s_audio:
  - id: i2s_shared
    i2s_lrclk_pin: GPIO33
    i2s_bclk_pin: GPIO34
    i2s_mclk_pin: GPIO0
    access_mode: duplex

adf_pipeline:
  - platform: i2s_audio
    type: audio_out
    id: adf_i2s_out
    i2s_audio_id: i2s_shared
    i2s_dout_pin: GPIO13
    adf_alc: false
    dac:
      model: aw88298
      address: 0x36
      enable_pin:
        aw9523: aw9523_1
        port: 0
        pin: 2
        mode:
          output: true
    sample_rate: 16000
    bits_per_sample: 16bit
    fixed_settings: true


  - platform: i2s_audio
    type: audio_in
    id: adf_i2s_in
    i2s_audio_id: i2s_shared
    i2s_din_pin: GPIO14
    pdm: false
    adc:
      model: es7210
      address: 0x40
    bits_per_sample: 16bit
    fixed_settings: true


microphone:
  - platform: adf_pipeline
    id: adf_microphone
    keep_pipeline_alive: true
    pipeline:
      - adf_i2s_in
      - resampler
      - self


media_player:
  - platform: adf_pipeline
    id: adf_media_player
    name: s3-dev_media_player
    internal: false
    keep_pipeline_alive: true
    pipeline:
      - self
      - resampler
      - adf_i2s_out
```


## Notes:
* using the same element in two pipelines (e.g. using adf_i2s_out in the speaker and the media_player) is not supported yet
* using the adf_pipeline component disables the verification of server certificates by setting the idf-sdk option "CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY". This is quick and dirty hack for allowing streaming from internet radio stations, be aware of the potential security issue.
