#include "audioManager.h"

int alsa_pcm_init(snd_pcm_t **pcm_handle, unsigned int sample_rate,
                  int channels) {

  int err;

  err = snd_pcm_open(pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) {
    fprintf(stderr, "Something went wrong with opening the PCM-Device: %s\n",
            snd_strerror(err));

    return err;
  }

  snd_pcm_hw_params_t *params;
  snd_pcm_hw_params_malloc(&params);
  snd_pcm_hw_params_any(*pcm_handle, params);

  // Interleaved Access-mode
  snd_pcm_hw_params_set_access(*pcm_handle, params,
                               SND_PCM_ACCESS_RW_INTERLEAVED);

  snd_pcm_hw_params_set_format(*pcm_handle, params, SND_PCM_FORMAT_FLOAT_LE);

  // Count of channels (stereo)
  snd_pcm_hw_params_set_channels(*pcm_handle, params, channels);

  // sample-rate
  unsigned int rate = sample_rate;
  snd_pcm_hw_params_set_rate_near(*pcm_handle, params, &rate, 0);

  snd_pcm_uframes_t frames = 1024;
  snd_pcm_hw_params_set_period_size_near(*pcm_handle, params, &frames, 0);

  err = snd_pcm_hw_params(*pcm_handle, params);
  if (err < 0) {
    fprintf(
        stderr,
        "Something went wrong with setting up the hardware-parameters: %s\n",
        snd_strerror(err));
    snd_pcm_hw_params_free(params);

    return err;
  }
  snd_pcm_hw_params_free(params);

  return 0;
}