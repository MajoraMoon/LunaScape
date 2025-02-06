#include "renderer.h"

void initRenderer(Renderer *renderer, int texWidth, int texHeight) {

  SDL_GL_SetSwapInterval(1);

  // vertices, quadrangle out of two triangles
  float vertices[] = {
      // Positions       // TexCoords (t inverted)
      // x     y     s     t
      -1.0f, -1.0f, 0.0f, 1.0f, // bottom left
      1.0f,  -1.0f, 1.0f, 1.0f, // bottom right
      1.0f,  1.0f,  1.0f, 0.0f, // top right
      -1.0f, 1.0f,  0.0f, 0.0f  // top left
  };

  // ebo, tells how the vertices should be "ordered"
  unsigned int indices[] = {0, 1, 2, 2, 3, 0};

  // create Vertex array objects, Vertex buffer objects and Element buffer
  // object
  glGenVertexArrays(1, &renderer->vao);
  glGenBuffers(1, &renderer->vbo);
  glGenBuffers(1, &renderer->ebo);

  // activates the vao, everything below will be referenced to this vao
  glBindVertexArray(renderer->vao);

  // activates the vbo, glBufferData below will be referenced to this vbo
  glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // activates the e bo, glBufferData below will be referenced to this ebo
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  // Position attrib (location 0)
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // TexCoords attrib (location 1)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));

  // activates Vertex-attribs "Tex Coords"
  glEnableVertexAttribArray(1);

  // unbined the vao
  glBindVertexArray(0);

  // generates a texture ID for OpenGL
  glGenTextures(1, &renderer->texture);

  // activates the texture and setting up all attribs for this texture
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

  // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  // glClear(GL_COLOR_BUFFER_BIT);

  // frameYUV->data uses RGB not YUV from the frames
  glBindTexture(GL_TEXTURE_2D, renderer->texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srcWidth, srcHeight, GL_RGB,
                  GL_UNSIGNED_BYTE, videoFrame->frameYUV->data[0]);

  useShader(&renderer->shader);
  glBindVertexArray(renderer->vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void updateVideoTranformation(Renderer *renderer, int windowWidth,
                              int windowHeight, int videoWidth,
                              int videoHeight) {

  // set OpenGL viewport on the current window size
  glViewport(0, 0, windowWidth, windowHeight);

  // calculate the aspect ratio of window and video
  float aspectWindow = (float)windowWidth / windowHeight;
  float aspectVideo = (float)videoWidth / videoHeight;

  // init scaling factors
  float scaleX = 1.0f, scaleY = 1.0f;

  if (aspectVideo > aspectWindow) {
    // if video is wider, scale height
    scaleY = aspectWindow / aspectVideo;
  } else {
    // if video is narrow, scale width
    scaleX = aspectVideo / aspectWindow;
  }

  // a 4x4 transform matrix
  // clang-format off
    float transform[16] = {
        scaleX, 0.0f,   0.0f, 0.0f,
        0.0f,   scaleY, 0.0f, 0.0f,
        0.0f,   0.0f,   1.0f, 0.0f,
        0.0f,   0.0f,   0.0f, 1.0f
    };
  // clang-format on

  // use shader and set uniform varible "transform" from the vertex shader
  glUseProgram(renderer->shader.ID);
  int transformLoc = glGetUniformLocation(renderer->shader.ID, "transform");
  glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transform);
}

void cleanupRenderer(Renderer *renderer) {
  glDeleteVertexArrays(1, &renderer->vao);
  glDeleteBuffers(1, &renderer->vbo);
  glDeleteTextures(1, &renderer->texture);
}