#include <main.h>

// window size when executing the program
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

volatile double audio_clock = 0.0;

typedef struct AudioThreadData {
  AudioContainer *audio;
  aFrame *audioFrame;
  int channels;
  volatile bool running;
  unsigned int sample_rate;
  volatile double *audio_clock_ptr;
} AudioThreadData;

static int paCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo *timeInfo,
                      PaStreamCallbackFlags statusFlags, void *userData) {
  (void)inputBuffer; // Verhindert Warnungen über ungenutzte Variablen.
  (void)timeInfo;
  (void)statusFlags;

  AudioThreadData *atd = (AudioThreadData *)userData;
  float *out = (float *)outputBuffer;
  unsigned long framesCopied = 0;

  // Lautstärke-Faktor (z. B. 0.5 für 50% der Originallautstärke)
  const float volume = 0.5f;

  while (framesCopied < framesPerBuffer) {
    if (audio_container_get_frame(atd->audio, atd->audioFrame)) {
      int framesAvailable =
          atd->audioFrame->convertedDataSize / (sizeof(float) * atd->channels);
      int framesToCopy = (framesAvailable < (framesPerBuffer - framesCopied))
                             ? framesAvailable
                             : (framesPerBuffer - framesCopied);

      // Kopiere und skaliere die Samples zeilenweise:
      for (int i = 0; i < framesToCopy * atd->channels; i++) {
        // Multipliziere jeden Sample mit dem Lautstärke-Faktor
        out[framesCopied * atd->channels + i] =
            atd->audioFrame->convertedData[0][i] * volume;
      }
      framesCopied += framesToCopy;
      // Falls dein Frame noch extra Samples enthält, könntest du diese hier
      // zwischenspeichern.
    } else {
      // Falls kein Frame verfügbar, fülle den Rest mit Stille.
      for (unsigned long i = framesCopied; i < framesPerBuffer; i++) {
        for (int ch = 0; ch < atd->channels; ch++) {
          out[i * atd->channels + ch] = 0.0f;
        }
      }
      framesCopied = framesPerBuffer;
    }
  }
  // Aktualisiere die Audio-Uhr (in Sekunden)
  *(atd->audio_clock_ptr) += ((double)framesCopied / atd->sample_rate);
  return paContinue;
}

int main(int argc, char *argv[]) {

  char *video_file = KDE_Plasma_select_video_file();
  if (!video_file || video_file[0] == '\0') {
    SDL_Log("No videofile selected. Closing programm.");
    if (video_file) {
      free(video_file);
    }
    return -1;
  }

  VideoContainer *video = init_video_container(video_file, false);
  if (!video) {
    SDL_Log("Failed to init video\n");
    free(video_file);
    return -1;
  }

  vFrame *videoFrame = init_video_frames(video);
  if (!videoFrame) {
    SDL_Log("Failed to init videoFrames");
    return -1;
  }

  AudioContainer *audio = init_audio_container(video_file);
  if (!audio) {
    SDL_Log("Fehler beim Initialisieren des Audios.");
    free(video_file);
    SDL_Quit();
    return -1;
  }

  aFrame *audioFrame = init_audio_frames(audio);
  if (!audioFrame) {
    SDL_Log("Fehler beim Initialisieren der AudioFrames.");
    free_audio_data(audio);
    SDL_Quit();
    return -1;
  }
  free(video_file);

  // init SDL3 window with OpenGL context.
  SDL_Window *window =
      initWayWindowGL("LunaScape", "0.1", SCR_WIDTH, SCR_HEIGHT, true);

  if (window == NULL) {
    SDL_Log("Something went wrong in setting up a SDL window.\n");
    return -1;
  }

  SDL_GLContext glContext = initOpenGLContext_and_glad(window);

  if (!glContext) {
    SDL_Log("Something went wrong in setting up a glContext.\n");
    return -1;
  }
  /*
    AudioManager audioManager;
    if (audio_manager_init(&audioManager, video_file) < 0) {
      SDL_Log("Failed to initialize audio manager");
      free(video_file);
      return -1;
    } */

  Renderer renderer;
  initRenderer(&renderer, video->pCodecCtx->width, video->pCodecCtx->height);

  // --- PortAudio Initialization ---
  unsigned int sample_rate = audio->pCodecCtx->sample_rate;
  int channels = 2; // stereo

  AudioThreadData atd;
  atd.audio = audio;
  atd.audioFrame = audioFrame;
  atd.channels = channels;
  atd.running = true;
  atd.sample_rate = sample_rate;
  atd.audio_clock_ptr = &audio_clock;

  PaStream *paStream;
  // Instead of using ALSA, we initialize PortAudio via our helper function.
  if (portaudio_init(&paStream, sample_rate, channels, paCallback, &atd) < 0) {
    SDL_Log("Failed to initialize PortAudio.");
    cleanupRenderer(&renderer);
    cleanupWindow(window, glContext);
    free_video_frames(videoFrame);
    free_video_data(video);
    free_audio_frames(audioFrame);
    free_audio_data(audio);
    SDL_Quit();
    return -1;
  }
  // --- End PortAudio Initialization ---

  bool running = true;

  uint64_t start_time = SDL_GetTicksNS();
  uint64_t pauseStart = 0;

  // press "F" for activating Fullscreen
  bool isFullscreen = false;

  // main render loop
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {

      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }

      // Key Press down
      if (event.type == SDL_EVENT_KEY_DOWN) {

        // If in fullscreen mode and pressed escape key, leave fullscreen
        // mode. when not in fullscreenmode and pressed escape key, close
        // application.
        if (isFullscreen && event.key.key == SDLK_ESCAPE) {

          SDL_SetWindowFullscreen(window, false);
          isFullscreen = !isFullscreen;

        } else if (event.key.key == SDLK_ESCAPE) {
          running = false;
        }

        if (event.key.key == SDLK_F) {
          isFullscreen = !isFullscreen;
          if (isFullscreen) {
            SDL_SetWindowFullscreen(window, true);
          } else {
            SDL_SetWindowFullscreen(window, false);
          }
        }

        if (event.key.key == SDLK_SPACE) {
          if (!video->paused) {
            pauseStart = SDL_GetTicksNS();
            video->paused = true;
          } else {
            uint64_t pauseDuration = SDL_GetTicksNS() - pauseStart;
            start_time += pauseDuration;
            video->paused = false;
          }
        }

        if (event.key.key == SDLK_R) {
          // if not paused, before reloading pause video so the it's not
          // decoding in the background
          if (!video->paused) {
            pauseStart = SDL_GetTicksNS();
            video->paused = true;
            SDL_Delay(10);
          }

          // saving old dimensions, for a possible new renderer.
          int oldWidth = video->pCodecCtx->width;
          int oldHeight = video->pCodecCtx->height;

          if (!reload_video(&video, &videoFrame, &start_time)) {
            // if the video selection was cancelled, unpause video
            uint64_t pauseDuration = SDL_GetTicksNS() - pauseStart;
            start_time += pauseDuration;
            video->paused = false;
          }

          // if the dimensions of the old video are not the same as the
          // dimensions of the new video, the texures in the renderer need to be
          // updated to these dimensions too
          if (video && (video->pCodecCtx->width != oldWidth ||
                        video->pCodecCtx->height != oldHeight)) {
            SDL_Log("Video resolution changed: old: %dx%d, new: %dx%d. "
                    "Reinitializing renderer...",
                    oldWidth, oldHeight, video->pCodecCtx->width,
                    video->pCodecCtx->height);
            cleanupRenderer(&renderer);
            initRenderer(&renderer, video->pCodecCtx->width,
                         video->pCodecCtx->height);
          }
        }
      }
    }

    // responsible for holding the aspect ratio of the video right
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    updateVideoTranformation(&renderer, windowWidth, windowHeight,
                             video->pCodecCtx->width, video->pCodecCtx->height);

    // when video is paused, then the current frame will be rendered with less
    // ressources used. When not paused, it uses two always alternating
    // buffers pixel buffer objects.
    if (!video->paused) {

      if (video_container_get_frame(video, videoFrame)) {
        // Sync the render loops framerate with the one from the given Video.
        // Doing magic basically.
        int64_t pts = videoFrame->frame->pts;
        double timestamp =
            pts *
            av_q2d(
                video->pFormatCtx->streams[video->videoStreamIndex]->time_base);

        double current_time_sec = (double)(SDL_GetTicksNS() - start_time) / 1e9;
        double wait_time = timestamp - current_time_sec;

        if (wait_time > 0.001) { // Only delays when the time difference is
                                 // bigger than 1ms
          SDL_Delay((uint32_t)(wait_time * 1000));
        }

        renderFrameWithPBO(&renderer, video->pCodecCtx->width,
                           video->pCodecCtx->height, videoFrame);
      } else {
        // When no frames avaiable anymore, start the video from the beginning
        av_seek_frame(video->pFormatCtx, video->videoStreamIndex, 0,
                      AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(video->pCodecCtx);
      }

    } else {

      renderFrameWithoutUpdate(&renderer);
      SDL_Delay(200);
    }

    SDL_GL_SwapWindow(window);
  }

  // --- PortAudio Cleanup ---
  PaError paErr = Pa_StopStream(paStream);
  if (paErr != paNoError) {
    SDL_Log("PortAudio stop stream error: %s", Pa_GetErrorText(paErr));
  }
  Pa_CloseStream(paStream);
  Pa_Terminate();
  // --- End PortAudio Cleanup ---
  free_video_frames(videoFrame);
  free_video_data(video);
  cleanupRenderer(&renderer);
  cleanupWindow(window, glContext);
  free_audio_frames(audioFrame);
  free_audio_data(audio);

  SDL_Quit();
  return 0;
}
