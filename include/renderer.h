#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>
#include <video_player.h>

typedef struct Renderer {
  SDL_Renderer *renderer;
  SDL_Texture *texture;
} Renderer;

Renderer *init_renderer(SDL_Window *window, int vWidth, int vHeight);
void renderer_render_frames(Renderer *renderer, vFrame *videoFrame);
void free_renderer(Renderer *renderer);

#endif