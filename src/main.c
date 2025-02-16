#include <main.h>

// window size when executing the program
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

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

  AudioManager audioManager;
  if (audio_manager_init(&audioManager, video_file) < 0) {
    SDL_Log("Failed to initialize audio manager");
    free(video_file);
    return -1;
  }
  free(video_file);

  Renderer renderer;
  initRenderer(&renderer, video->pCodecCtx->width, video->pCodecCtx->height);

  bool running = true;

  uint64_t pauseStart = 0;

  // press "F" for activating Fullscreen
  bool isFullscreen = false;

  audio_manager_start(&audioManager);
  uint64_t start_time = SDL_GetTicksNS();
  // main render loop
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {

      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }

      // Key Press down
      if (event.type == SDL_EVENT_KEY_DOWN) {

        // If in fullscreen mode and pressed escape key, leave fullscreen mode.
        // when not in fullscreenmode and pressed escape key, close application.
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
            SDL_PauseAudioStreamDevice(audioManager.audioStream);

          } else {
            uint64_t pauseDuration = SDL_GetTicksNS() - pauseStart;
            start_time += pauseDuration;
            video->paused = false;
            SDL_ResumeAudioStreamDevice(audioManager.audioStream);
          }
        }

        if (event.key.key == SDLK_R) {
          // if not paused, before reloading pause video so the it's not
          // decoding in the background
          if (!video->paused) {
            pauseStart = SDL_GetTicksNS();
            video->paused = true;
            SDL_PauseAudioStreamDevice(audioManager.audioStream);
            SDL_Delay(10);
          }

          // saving old dimensions, for a possible new renderer.
          int oldWidth = video->pCodecCtx->width;
          int oldHeight = video->pCodecCtx->height;

          if (!reload_video_and_audio(&video, &videoFrame, &audioManager,
                                      &start_time)) {
            // if the video selection was cancelled, unpause video
            uint64_t pauseDuration = SDL_GetTicksNS() - pauseStart;
            start_time += pauseDuration;
            video->paused = false;
            SDL_ResumeAudioStreamDevice(audioManager.audioStream);
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

        if (event.key.key == SDLK_M) {
          audioManager.muted = !audioManager.muted;
        }
      }
    }

    // responsible for holding the aspect ratio of the video right
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    updateVideoTranformation(&renderer, windowWidth, windowHeight,
                             video->pCodecCtx->width, video->pCodecCtx->height);

    // when video is paused, then the current frame will be rendered with less
    // ressources used. When not paused, it uses two always alternating buffers
    // pixel buffer objects.
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
        double audio_time_sec =
            (double)(SDL_GetAudioStreamQueued(audioManager.audioStream)) /
            (audioManager.audio->pCodecCtx->sample_rate *
             audioManager.audio->pCodecCtx->ch_layout.nb_channels *
             av_get_bytes_per_sample(
                 audioManager.audio->pCodecCtx->sample_fmt));

        double wait_time = timestamp - current_time_sec;

        if (wait_time > 0.005) { // 5ms Toleranz
          SDL_Delay((uint32_t)(wait_time * 1000));
        } else if (audio_time_sec > timestamp) {
          SDL_Delay((uint32_t)((audio_time_sec - timestamp) * 1000 / 2));
        }

        renderFrameWithPBO(&renderer, video->pCodecCtx->width,
                           video->pCodecCtx->height, videoFrame);
      } else {
        // When no frames avaiable anymore, start the video from the beginning
        av_seek_frame(video->pFormatCtx, video->videoStreamIndex, 0,
                      AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(video->pCodecCtx);

        av_seek_frame(audioManager.audio->pFormatCtx,
                      audioManager.audio->audioStreamIndex, 0,
                      AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(audioManager.audio->pCodecCtx);
      }

    } else {

      renderFrameWithoutUpdate(&renderer);

      SDL_Delay(200);
    }

    SDL_GL_SwapWindow(window);
  }

  audio_manager_stop(&audioManager);
  audio_manager_cleanup(&audioManager);
  free_video_frames(videoFrame);
  free_video_data(video);
  cleanupRenderer(&renderer);
  cleanupWindow(window, glContext);

  return 0;
}