import esphome.codegen as cg

CONF_PRIMARY = "primary"
CONF_SECONDARY = "secondary"
i2s_mode_t = cg.global_ns.enum("i2s_mode_t")
I2S_CLK_MODE_OPTIONS = {
    CONF_PRIMARY: i2s_mode_t.I2S_MODE_MASTER,  # NOLINT
    CONF_SECONDARY: i2s_mode_t.I2S_MODE_SLAVE,  # NOLINT
}


i2s_dac_mode_t = cg.global_ns.enum("i2s_dac_mode_t")
INTERNAL_DAC_OPTIONS = {
    "left": i2s_dac_mode_t.I2S_DAC_CHANNEL_LEFT_EN,
    "right": i2s_dac_mode_t.I2S_DAC_CHANNEL_RIGHT_EN,
    "stereo": i2s_dac_mode_t.I2S_DAC_CHANNEL_BOTH_EN,
}

i2s_channel_fmt_t = cg.global_ns.enum("i2s_channel_fmt_t")
CHANNEL_FORMAT = {
    # Only load data in left channel (mono mode)
    "left": i2s_channel_fmt_t.I2S_CHANNEL_FMT_ONLY_LEFT,
    # Only load data in right channel (mono mode)
    "right": i2s_channel_fmt_t.I2S_CHANNEL_FMT_ONLY_RIGHT,
    # Separated left and right channel
    "right_left": i2s_channel_fmt_t.I2S_CHANNEL_FMT_RIGHT_LEFT,
    # Load right channel data in both two channels
    "all_right": i2s_channel_fmt_t.I2S_CHANNEL_FMT_ALL_RIGHT,
    # Load left channel data in both two channels
    "all_left": i2s_channel_fmt_t.I2S_CHANNEL_FMT_ALL_LEFT,
}

i2s_bits_per_sample_t = cg.global_ns.enum("i2s_bits_per_sample_t")
BITS_PER_SAMPLE = {
    16: i2s_bits_per_sample_t.I2S_BITS_PER_SAMPLE_16BIT,
    24: i2s_bits_per_sample_t.I2S_BITS_PER_SAMPLE_24BIT,
    32: i2s_bits_per_sample_t.I2S_BITS_PER_SAMPLE_32BIT,
}

i2s_bits_per_chan_t = cg.global_ns.enum("i2s_bits_per_chan_t")
BITS_PER_CHANNEL = {
    "default": i2s_bits_per_chan_t.I2S_BITS_PER_CHAN_DEFAULT,
    8: i2s_bits_per_chan_t.I2S_BITS_PER_CHAN_8BIT,
    16: i2s_bits_per_chan_t.I2S_BITS_PER_CHAN_16BIT,
    24: i2s_bits_per_chan_t.I2S_BITS_PER_CHAN_24BIT,
    32: i2s_bits_per_chan_t.I2S_BITS_PER_CHAN_32BIT,
}
