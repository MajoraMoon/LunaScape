#include <renderer.h>
#include <video_player.h>
#include <wayWindow.h>

#define VIDEO_FILE "../test.mp4"

int main(int argc, char *argv[]) {

  VideoPlayer *videoPlayer = init_video_player(VIDEO_FILE);

  if (!videoPlayer) {
    SDL_Log("Something went wrong in setting up the Video player.\n");
    return -1;
  }

  SDL_Window *window = initWayWindow("LunaScape", "0.1", 1920, 1080, true);

  if (!window) {
    SDL_Log("Something went wrong in setting up a SDL window.\n");
    return -1;
  }

  Renderer *renderer = init_renderer(window, videoPlayer->pCodecCtx->width,
                                     videoPlayer->pCodecCtx->height);

  if (!renderer) {
    SDL_Log("Something went wrong in setting up the renderer.\n");
    return -1;
  }

  vFrame *videoFrame = init_video_frames(videoPlayer);

  if (!videoFrame) {
    SDL_Log(
        "Something went wrong in setting up the Frame rendering process.\n");
    return -1;
  }

  bool running = true;
  printf("Frame Konvertierung: %d x %d -> %d x %d\n",
         videoPlayer->pCodecCtx->width, videoPlayer->pCodecCtx->height,
         videoPlayer->pCodecCtx->width, videoPlayer->pCodecCtx->height);

  uint32_t start_time = SDL_GetTicks();
  while (running) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
        running = false;
      }
    }

    uint32_t current_time = SDL_GetTicks() - start_time;
    double current_time_sec = current_time / 1000.0;

    if (video_player_get_frame(videoPlayer, videoFrame)) {

      int64_t pts = videoFrame->frame->pts;
      double timestamp =
          pts *
          av_q2d(videoPlayer->pFormatCtx->streams[videoPlayer->videoStreamIndex]
                     ->time_base);

      if (timestamp > current_time_sec) {
        uint32_t wait_time = (timestamp - current_time_sec) * 1000; // in ms
        SDL_Delay(wait_time);
      }

      renderer_render_frames(renderer, videoFrame);
    } else {
      av_seek_frame(videoPlayer->pFormatCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
      avcodec_flush_buffers(videoPlayer->pCodecCtx);
    }
  }

  free_video_player(videoPlayer);
  free_video_frames(videoFrame);
  free_renderer(renderer);
  cleanupWindow(window);

  return 0;
}
