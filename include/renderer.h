#ifndef RENDERER_H
#define RENDERER_H

// clang-format off

#include <glad/glad.h>
#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>

#include "shader.h"
#include "mediaLoader.h"

// clang-format on

typedef struct Renderer {

  GLuint vao;
  GLuint vbo;
  GLuint ebo;
  GLuint texture;
  Shader shader;

} Renderer;

// setting up data before the actual render loop
void initRenderer(Renderer *renderer, int texWidth, int texHeight);

// use in render loop, OpenGL actual rendering
void renderFrame(Renderer *renderer, unsigned int srcWidth,
                 unsigned int srcHeight, vFrame *videoFrame);

void updateVideoTranformation(Renderer *renderer, int windowWidth,
                              int windowHeight, int videoWidth,
                              int videoHeight);

// Destroy data from OpenGL
void cleanupRenderer(Renderer *renderer);

#endif