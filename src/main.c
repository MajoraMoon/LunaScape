#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct wayWindow {
  SDL_Window *window;
  SDL_GPUDevice *GPUDevice;
  SDL_PropertiesID props;
} wayWindow;

typedef struct PositionColorVertex {
  float x, y, z;
  Uint8 r, g, b, a;
} PositionColorVertex;

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *shaderFile,
                          Uint32 samplerCount, Uint32 uniformBufferCount,
                          Uint32 storageBufferCount,
                          Uint32 storageTextureCount) {

  // stage of the shader. So is it a vertex or fragment shader?
  SDL_GPUShaderStage stage;

  // The shaders should have a ".vert" or ".frag" in their filenames.
  if (SDL_strstr(shaderFile, ".vert")) {
    stage = SDL_GPU_SHADERSTAGE_VERTEX;
  } else if (SDL_strstr(shaderFile, ".frag")) {
    stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
  } else {
    SDL_Log("Invalid shader stage!");
    return NULL;
  }

  // compiled vulcan shader used
  SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;
  const char *entrypoint = "main";
  char fullPath[256];
  SDL_snprintf(fullPath, sizeof(fullPath), "../shader/compiled/%s.spv",
               shaderFile);

  // load actual shader file
  size_t codeSize;
  void *code = SDL_LoadFile(fullPath, &codeSize);
  if (code == NULL) {
    SDL_Log("Failed to load shader from disk! %s", fullPath);
    return NULL;
  }

  SDL_GPUShaderCreateInfo shaderInfo = {

      .code = code,
      .code_size = codeSize,
      .entrypoint = entrypoint,
      .format = format,
      .stage = stage,
      .num_samplers = samplerCount,
      .num_uniform_buffers = uniformBufferCount,
      .num_storage_buffers = storageBufferCount,
      .num_storage_textures = storageTextureCount

  };

  SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shaderInfo);

  if (shader == NULL) {
    SDL_Log("Failed to create shader!");
    SDL_free(code);
    return NULL;
  }
  SDL_free(code);
  return shader;
}

int main(int argc, char *argv[]) {

  // main init of SDL
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());

    return -1;
  }

  wayWindow wayWindow;

  // creating a GPU Device with Vulcan shader syntax
  wayWindow.GPUDevice =
      SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);

  if (wayWindow.GPUDevice == NULL) {
    SDL_Log("GPUCreateDevice failed %s", SDL_GetError());
    return -1;
  }

  // Preparing properties for creating the window
  wayWindow.props = SDL_CreateProperties();

  if (wayWindow.props == 0) {
    SDL_Log("Unable to create properties: %s", SDL_GetError());

    return -1;
  }

  SDL_SetStringProperty(wayWindow.props, SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                        "WayWindow");

  SDL_SetBooleanProperty(wayWindow.props,
                         SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);

  SDL_SetNumberProperty(wayWindow.props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,
                        1080);
  SDL_SetNumberProperty(wayWindow.props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER,
                        720);

  SDL_SetBooleanProperty(wayWindow.props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN,
                         true);

  // later implementating a different wayland window session. If activated now,
  // no window will be displayed.
  //  SDL_SetNumberProperty(props,
  //  SDL_PROP_WINDOW_CREATE_WAYLAND_SURFACE_ROLE_CUSTOM_BOOLEAN, true);

  wayWindow.window = SDL_CreateWindowWithProperties(wayWindow.props);

  if (wayWindow.window == NULL) {
    SDL_Log("CreateWindow failed: %s", SDL_GetError());
    return -1;
  }

  // Connect created window with GPU Device
  if (!SDL_ClaimWindowForGPUDevice(wayWindow.GPUDevice, wayWindow.window)) {
    SDL_Log("GPUClaimWindow failed %s", SDL_GetError());
    return -1;
  }

  // Create vertex Shaders
  SDL_GPUShader *vertexShader =
      LoadShader(wayWindow.GPUDevice, "PositionColor.vert", 0, 0, 0, 0);

  if (vertexShader == NULL) {
    SDL_Log("Failed to create vertex shader!");
    return -1;
  }
  // Create fragment Shaders
  SDL_GPUShader *fragmentShader =
      LoadShader(wayWindow.GPUDevice, "SolidColor.frag", 0, 0, 0, 0);
  if (fragmentShader == NULL) {
    SDL_Log("Failed to create fragment shader!");
    return -1;
  }

  // Create the pipeline
  SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
      .target_info =
          {
              .num_color_targets = 1,
              .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
                  .format = SDL_GetGPUSwapchainTextureFormat(
                      wayWindow.GPUDevice, wayWindow.window),
              }},
          },
      // This is set up to match the vertex shader layout!
      .vertex_input_state =
          (SDL_GPUVertexInputState){
              .num_vertex_buffers = 1,
              .vertex_buffer_descriptions =
                  (SDL_GPUVertexBufferDescription[]){
                      {.slot = 0,
                       .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                       .instance_step_rate = 0,
                       .pitch = sizeof(PositionColorVertex)}},
              .num_vertex_attributes = 2,
              .vertex_attributes =
                  (SDL_GPUVertexAttribute[]){
                      {.buffer_slot = 0,
                       .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                       .location = 0,
                       .offset = 0},
                      {.buffer_slot = 0,
                       .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
                       .location = 1,
                       .offset = sizeof(float) * 3}}},
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .vertex_shader = vertexShader,
      .fragment_shader = fragmentShader};

  SDL_GPUGraphicsPipeline *Pipeline =
      SDL_CreateGPUGraphicsPipeline(wayWindow.GPUDevice, &pipelineCreateInfo);
  if (Pipeline == NULL) {
    SDL_Log("Failed to create pipeline!");
    return -1;
  }

  SDL_ReleaseGPUShader(wayWindow.GPUDevice, vertexShader);
  SDL_ReleaseGPUShader(wayWindow.GPUDevice, fragmentShader);

  SDL_GPUBuffer *VertexBuffer = SDL_CreateGPUBuffer(
      wayWindow.GPUDevice,
      &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                 .size = sizeof(PositionColorVertex) * 3});

  // To get data into the vertex buffer, we have to use a transfer buffer
  SDL_GPUTransferBuffer *transferBuffer = SDL_CreateGPUTransferBuffer(
      wayWindow.GPUDevice, &(SDL_GPUTransferBufferCreateInfo){
                               .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                               .size = sizeof(PositionColorVertex) * 3});

  PositionColorVertex *transferData =
      SDL_MapGPUTransferBuffer(wayWindow.GPUDevice, transferBuffer, false);

  transferData[0] = (PositionColorVertex){-1, -1, 0, 255, 0, 0, 255};
  transferData[1] = (PositionColorVertex){1, -1, 0, 0, 255, 0, 255};
  transferData[2] = (PositionColorVertex){0, 1, 0, 0, 0, 255, 255};

  SDL_UnmapGPUTransferBuffer(wayWindow.GPUDevice, transferBuffer);

  // Upload the transfer data to the vertex buffer
  SDL_GPUCommandBuffer *uploadCmdBuf =
      SDL_AcquireGPUCommandBuffer(wayWindow.GPUDevice);
  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

  SDL_UploadToGPUBuffer(
      copyPass,
      &(SDL_GPUTransferBufferLocation){.transfer_buffer = transferBuffer,
                                       .offset = 0},
      &(SDL_GPUBufferRegion){.buffer = VertexBuffer,
                             .offset = 0,
                             .size = sizeof(PositionColorVertex) * 3},
      false);

  SDL_EndGPUCopyPass(copyPass);
  SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
  SDL_ReleaseGPUTransferBuffer(wayWindow.GPUDevice, transferBuffer);

  int quit = false;

  // main loop - keyevents and rendering
  while (!quit) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
      }

      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
        quit = true;
      }
    }

    SDL_GPUCommandBuffer *cmdbuf =
        SDL_AcquireGPUCommandBuffer(wayWindow.GPUDevice);
    if (cmdbuf == NULL) {
      SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
      return -1;
    }

    SDL_GPUTexture *swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, wayWindow.window,
                                               &swapchainTexture, NULL, NULL)) {
      SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
      return -1;
    }

    if (swapchainTexture != NULL) {
      SDL_GPUColorTargetInfo colorTargetInfo = {0};
      colorTargetInfo.texture = swapchainTexture;
      colorTargetInfo.clear_color = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};
      colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
      colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

      SDL_GPURenderPass *renderPass =
          SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);

      SDL_BindGPUGraphicsPipeline(renderPass, Pipeline);
      SDL_BindGPUVertexBuffers(
          renderPass, 0,
          &(SDL_GPUBufferBinding){.buffer = VertexBuffer, .offset = 0}, 1);
      SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

      SDL_EndGPURenderPass(renderPass);
    }

    SDL_SubmitGPUCommandBuffer(cmdbuf);
  }

  // Destroy everything when done
  SDL_ReleaseGPUGraphicsPipeline(wayWindow.GPUDevice, Pipeline);
  SDL_ReleaseGPUBuffer(wayWindow.GPUDevice, VertexBuffer);
  SDL_ReleaseWindowFromGPUDevice(wayWindow.GPUDevice, wayWindow.window);
  SDL_DestroyWindow(wayWindow.window);
  SDL_DestroyGPUDevice(wayWindow.GPUDevice);

  SDL_Quit();
  SDL_Log("LunaScape ended successfully.\n");

  return 0;
}
