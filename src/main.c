#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct wayWindow {
  SDL_Window *window;
  SDL_GPUDevice *GPUDevice;
  SDL_PropertiesID props;
} wayWindow;

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

  // prints the avaiable GPU Drivers. It's not really written down anywhere (At
  // least I did not find anything about it). But testing it on a Linux and
  // Windows System the index is: Vulkan, DirectX 12, Metal?
  SDL_Log("%s", SDL_GetGPUDriver(0));

  // loading a texture fullscreen (test lol)

  // First load shaders
  SDL_GPUShader *vertexShader =
      LoadShader(wayWindow.GPUDevice, "fullscreen.vert", 0, 0, 0, 0);

  SDL_GPUShader *fragmentShader =
      LoadShader(wayWindow.GPUDevice, "fullscreen.frag", 1, 0, 0, 0);

  SDL_Surface *image = IMG_Load("../assets/test.png");

  if (image == NULL) {
    SDL_Log("Could not load image data.");
    return -1;
  }

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
  }

  // Destroy everything when done
  SDL_ReleaseWindowFromGPUDevice(wayWindow.GPUDevice, wayWindow.window);
  SDL_DestroyWindow(wayWindow.window);
  SDL_DestroyGPUDevice(wayWindow.GPUDevice);

  SDL_Quit();
  SDL_Log("LunaScape ended successfully.\n");

  return 0;
}
