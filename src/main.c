#include <linux/limits.h>
#include <main.h>

#define VIDEO_FILE "../video.mp4"

// window size when executing the program
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

char *get_project_root() {
  static char path[PATH_MAX];
  if (realpath("..", path)) {
    return path;
  }
  return NULL;
}

char *select_video_file(void) {
  char buffer[1024] = {0};
  char command[1200];

  char *project_root = get_project_root();
  if (!project_root) {
    SDL_Log("Could not define project root.");
    return NULL;
  }

  snprintf(command, sizeof(command),
           "kdialog --getopenfilename \"%s\" \"Videos (*.mp4 *.mkv *.avi *.mov "
           "*.webm *.flv)\"",
           project_root);

  FILE *fp = popen(command, "r");
  if (!fp) {
    SDL_Log("Error: popen() failed.");
    return NULL;
  }

  if (!fgets(buffer, sizeof(buffer), fp)) {
    SDL_Log("Error: No file selected.");
    pclose(fp);
    return NULL;
  }
  pclose(fp);

  buffer[strcspn(buffer, "\n")] = '\0';

  if (strlen(buffer) == 0) {
    SDL_Log("Error: No supported files selected.");
    return NULL;
  }

  char *filename = malloc(strlen(buffer) + 1);
  if (filename)
    strcpy(filename, buffer);

  return filename;
}

int main(int argc, char *argv[]) {

  char *video_file = select_video_file();
  if (!video_file || video_file[0] == '\0') {
    SDL_Log("No videofile selected. Closing programm.");
    return -1;
  }
  SDL_Log("Selected Videofile: %s", video_file);

  VideoContainer *video = init_video_container(video_file, false);
  if (!video) {
    SDL_Log("Failed to init video\n");

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

  Renderer renderer;
  initRenderer(&renderer, video->pCodecCtx->width, video->pCodecCtx->height);

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
          } else {
            uint64_t pauseDuration = SDL_GetTicksNS() - pauseStart;
            start_time += pauseDuration;
            video->paused = false;
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
    // ressources used.
    if (!video->paused) {
      // Sync the render loops framerate with the one from the given Video
      if (video_container_get_frame(video, videoFrame)) {
        int64_t pts = videoFrame->frame->pts;
        double timestamp =
            pts *
            av_q2d(
                video->pFormatCtx->streams[video->videoStreamIndex]->time_base);

        double current_time_sec = (double)(SDL_GetTicksNS() - start_time) / 1e9;
        double wait_time = timestamp - current_time_sec;

        if (wait_time >
            0.001) { // Only delays when the time difference is bigger than 1ms
          SDL_Delay((uint32_t)(wait_time * 1000));
        }

        // On my sytem, the frame render time is going down from 5ms to 2ms when
        // using pixel buffer objects.
        renderFrameWithPBO(&renderer, video->pCodecCtx->width,
                           video->pCodecCtx->height, videoFrame);
      } else {
        // When no frames avaiable anymore, start the video from the beginning
        av_seek_frame(video->pFormatCtx, video->videoStreamIndex, 0,
                      AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(video->pCodecCtx);
      }

      // rendering without any other performance boost stuff lol
    } else {

      renderFrameWithoutUpdate(&renderer);

      SDL_Delay(200);
    }

    SDL_GL_SwapWindow(window);
  }

  free_video_frames(videoFrame);
  free_video_data(video);
  cleanupRenderer(&renderer);
  cleanupWindow(window, glContext);

  return 0;
}
