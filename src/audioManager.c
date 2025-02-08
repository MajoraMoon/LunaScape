#include "audioManager.h"

// This is the audio thread function that continuously feeds audio data.
static int audio_thread_func(void *data) {
  AudioManager *am = (AudioManager *)data;
  while (am->running) {
    int available = SDL_GetAudioStreamAvailable(am->audioStream);
    if (available < am->buffer_threshold) {
      if (audio_container_get_frame(am->audio, am->audioFrame)) {
        int ret = SDL_PutAudioStreamData(am->audioStream,
                                         am->audioFrame->convertedData[0],
                                         am->audioFrame->convertedDataSize);
        if (ret < 0) {
          SDL_Log("SDL_PutAudioStreamData error: %s", SDL_GetError());
        }
      } else {
        // No more audio frames available.
        am->running = false;
        break;
      }
    } else {
      SDL_Delay(5);
    }
  }
  return 0;
}

int audio_manager_init(AudioManager *am, const char *filepath) {
  // Initialize FFmpeg audio container and frames.
  am->audio = init_audio_container(filepath);
  if (!am->audio) {
    SDL_Log("Failed to initialize audio container.");
    return -1;
  }
  am->audioFrame = init_audio_frames(am->audio);
  if (!am->audioFrame) {
    SDL_Log("Failed to initialize audio frames.");
    free_audio_data(am->audio);
    return -1;
  }

  // Setup SDL Audio Spec.
  SDL_AudioSpec audioSpec;
  audioSpec.channels = 2;           // Stereo output.
  audioSpec.format = SDL_AUDIO_F32; // 32-bit float samples.
  audioSpec.freq = am->audio->pCodecCtx->sample_rate;

  am->audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                              &audioSpec, NULL, NULL);
  if (!am->audioStream) {
    SDL_Log("Failed to open audio stream: %s", SDL_GetError());
    free_audio_frames(am->audioFrame);
    free_audio_data(am->audio);
    return -1;
  }
  SDL_ResumeAudioStreamDevice(am->audioStream);

  am->buffer_threshold = 16384; // This threshold worked well for you.
  am->running = false;
  am->audioThread = NULL;
  return 0;
}

void audio_manager_start(AudioManager *am) {
  am->running = true;
  am->audioThread = SDL_CreateThread(audio_thread_func, "AudioThread", am);
  if (!am->audioThread) {
    SDL_Log("Failed to create audio thread: %s", SDL_GetError());
    am->running = false;
  }
}

void audio_manager_stop(AudioManager *am) {
  am->running = false;
  if (am->audioThread) {
    SDL_WaitThread(am->audioThread, NULL);
    am->audioThread = NULL;
  }
}

void audio_manager_cleanup(AudioManager *am) {

  if (am->audioFrame) {
    free_audio_frames(am->audioFrame);
  }
  if (am->audio) {
    free_audio_data(am->audio);
  }
}