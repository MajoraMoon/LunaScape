#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct wayWindow {
  SDL_Window *window;
  SDL_GPUDevice *GPUDevice;
  SDL_PropertiesID props;
} wayWindow;

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

    SDL_GPUCommandBuffer *gpu_cmdbuffer =
        SDL_AcquireGPUCommandBuffer(wayWindow.GPUDevice);

    if (gpu_cmdbuffer == NULL) {
      SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
      return -1;
    }

    SDL_GPUTexture *swapchainTexture;

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(gpu_cmdbuffer, wayWindow.window,
                                               &swapchainTexture, NULL, NULL)) {
      SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
      return -1;
    }

    if (swapchainTexture != NULL) {
      SDL_GPUColorTargetInfo colorTargetInfo = {0};
      colorTargetInfo.texture = swapchainTexture;
      colorTargetInfo.clear_color = (SDL_FColor){0.3f, 0.4f, 0.5f, 1.0f};
      colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
      colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

      SDL_GPURenderPass *renderPass =
          SDL_BeginGPURenderPass(gpu_cmdbuffer, &colorTargetInfo, 1, NULL);
      SDL_EndGPURenderPass(renderPass);
    }

    SDL_SubmitGPUCommandBuffer(gpu_cmdbuffer);
  }

  // Destroy everything when done
  SDL_ReleaseWindowFromGPUDevice(wayWindow.GPUDevice, wayWindow.window);
  SDL_DestroyWindow(wayWindow.window);
  SDL_DestroyGPUDevice(wayWindow.GPUDevice);

  SDL_Quit();
  SDL_Log("LunaScape ended successfully.\n");

  return 0;
}
