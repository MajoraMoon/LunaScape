#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "mediaLoader.h" // Contains definitions for AudioContainer, aFrame, etc.
#include <SDL3/SDL.h>
#include <alsa/asoundlib.h>
#include <portaudio.h>

typedef struct AudioManager {
  AudioContainer *audio;
  aFrame *audioFrame;
  volatile bool running;
  int buffer_threshold;
} AudioManager;

int portaudio_init(PaStream **stream, unsigned int sample_rate, int channels,
                   PaStreamCallback *callback, void *userData);

#endif // AUDIO_MANAGER_H
