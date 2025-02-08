#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "mediaLoader.h" // Contains definitions for AudioContainer, aFrame, etc.
#include <SDL3/SDL.h>
#include <alsa/asoundlib.h>

typedef struct AudioManager {
  AudioContainer *audio;        // FFmpeg audio container.
  aFrame *audioFrame;           // Reusable audio frame structure.
  SDL_AudioStream *audioStream; // SDL3 audio stream device.
  SDL_Thread *audioThread;      // Audio thread pointer.
  volatile bool running;        // Flag to control the audio thread.
  int buffer_threshold;         // Threshold in bytes (e.g., 16384).
} AudioManager;

int alsa_pcm_init(snd_pcm_t **pcm_handle, unsigned int sample_rate,
                  int channels);

#endif // AUDIO_MANAGER_H
