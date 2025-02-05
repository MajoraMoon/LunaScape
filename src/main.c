#include <main.h>

#define VIDEO_FILE "../video3.mp4"

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

int main(int argc, char *argv[]) {

  VideoContainer *video = init_video_container(VIDEO_FILE);
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
      initWayWindowGL("LunaCore", "0.1", video->pCodecCtx->width,
                      video->pCodecCtx->height, true);

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
  uint32_t start_time = SDL_GetTicks();
  // main render loop
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {

      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }

      // Key Press down
      if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_ESCAPE) {
          running = false;
        }
      }
    }
    if (video_container_get_frame(video, videoFrame)) {
      // Timing (optional): Warte, wenn der Videotimestamp größer ist als die
      // verstrichene Zeit
      int64_t pts = videoFrame->frame->pts;
      double timestamp =
          pts *
          av_q2d(
              video->pFormatCtx->streams[video->videoStreamIndex]->time_base);
      double current_time_sec = (SDL_GetTicks() - start_time) / 1000.0;
      if (timestamp > current_time_sec) {
        SDL_Delay((uint32_t)((timestamp - current_time_sec) * 1000));
      }

      // Render mit OpenGL:
      renderFrame(&renderer, video->pCodecCtx->width, video->pCodecCtx->height,
                  videoFrame);
    } else {
      // Video zu Ende – Loop wieder von Anfang
      av_seek_frame(video->pFormatCtx, video->videoStreamIndex, 0,
                    AVSEEK_FLAG_BACKWARD);
      avcodec_flush_buffers(video->pCodecCtx);
    }

    SDL_GL_SwapWindow(window);
  }

  free_video_frames(videoFrame);
  free_video_data(video);
  cleanupRenderer(&renderer);
  cleanupWindow(window, glContext);

  return 0;
}
