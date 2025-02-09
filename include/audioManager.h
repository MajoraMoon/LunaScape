#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "mediaLoader.h" // Contains definitions for AudioContainer, aFrame, etc.
#include <SDL3/SDL.h>

typedef struct AudioManager {
  AudioContainer *audio;        // FFmpeg audio container.
  aFrame *audioFrame;           // Reusable audio frame structure.
  SDL_AudioStream *audioStream; // SDL3 audio stream device.
  SDL_Thread *audioThread;      // Audio thread pointer.
  volatile bool running;        // Flag to control the audio thread.
  int buffer_threshold;         // Threshold in bytes (e.g., 16384).
  bool muted;
} AudioManager;

// Initializes the audio manager from a file path.
int audio_manager_init(AudioManager *am, const char *filepath);

// Starts the audio processing thread.
void audio_manager_start(AudioManager *am);

// Stops the audio processing thread.
void audio_manager_stop(AudioManager *am);

// Cleans up all audio-related resources.
void audio_manager_cleanup(AudioManager *am);

#endif // AUDIO_MANAGER_H