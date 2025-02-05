#include "renderer.h"

void initRenderer(Renderer *renderer, int texWidth, int texHeight) {

  // VSync is on (at least on standard settings. This will deactivate it before
  // the rendering)
  SDL_GL_SetSwapInterval(0);

  float vertices[] = {
      // Positions       // TexCoords (angepasst: v invertiert)
      -1.0f, -1.0f, 0.0f, 1.0f, // unten links
      1.0f,  -1.0f, 1.0f, 1.0f, // unten rechts
      1.0f,  1.0f,  1.0f, 0.0f, // oben rechts
      -1.0f, 1.0f,  0.0f, 0.0f  // oben links
  };

  unsigned int indices[] = {0, 1, 2, 2, 3, 0};

  glGenVertexArrays(1, &renderer->vao);
  glGenBuffers(1, &renderer->vbo);
  glGenBuffers(1, &renderer->ebo);

  glBindVertexArray(renderer->vao);

  glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  // Position attrib (location 0)
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // TexCoords attrib (location 1)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);

  glGenTextures(1, &renderer->texture);
  glBindTexture(GL_TEXTURE_2D, renderer->texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);

  renderer->shader = createShader("../shader/vertexShader.vert",
                                  "../shader/fragmentShader.frag");
}

void renderFrame(Renderer *renderer, unsigned int srcWidth,
                 unsigned int srcHeight, vFrame *videoFrame) {
  glViewport(0, 0, srcWidth, srcHeight);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // frameYUV->data uses RGB not YUV from the frames
  glBindTexture(GL_TEXTURE_2D, renderer->texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srcWidth, srcHeight, GL_RGB,
                  GL_UNSIGNED_BYTE, videoFrame->frameYUV->data[0]);

  useShader(&renderer->shader);
  glBindVertexArray(renderer->vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void cleanupRenderer(Renderer *renderer) {
  glDeleteVertexArrays(1, &renderer->vao);
  glDeleteBuffers(1, &renderer->vbo);
  glDeleteTextures(1, &renderer->texture);
}