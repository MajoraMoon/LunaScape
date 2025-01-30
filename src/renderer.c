#include <renderer.h>

// vWidth and vHeight are the dimensions of the Video (from VideoPlayer)
Renderer *init_renderer(SDL_Window *window, int vWidth, int vHeight) {

  Renderer *renderer = (Renderer *)malloc(sizeof(Renderer));
  if (!renderer) {
    return NULL;
  }

  renderer->renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer->renderer) {
    free(renderer);
    return NULL;
  }

  renderer->texture =
      SDL_CreateTexture(renderer->renderer, SDL_PIXELFORMAT_YV12,
                        SDL_TEXTUREACCESS_STREAMING, vWidth, vHeight);

  return renderer;
}

void renderer_render_frames(Renderer *renderer, vFrame *videoFrame) {

  SDL_UpdateYUVTexture(
      renderer->texture, NULL, videoFrame->frameYUV->data[0],
      videoFrame->frameYUV->linesize[0], videoFrame->frameYUV->data[1],
      videoFrame->frameYUV->linesize[1], videoFrame->frameYUV->data[2],
      videoFrame->frameYUV->linesize[2]);

  SDL_RenderClear(renderer->renderer);
  SDL_RenderTexture(renderer->renderer, renderer->texture, NULL, NULL);
  SDL_RenderPresent(renderer->renderer);
}

void free_renderer(Renderer *renderer) {

  if (!renderer) {
    return;
  }

  SDL_DestroyTexture(renderer->texture);
  SDL_DestroyRenderer(renderer->renderer);
  free(renderer);
}