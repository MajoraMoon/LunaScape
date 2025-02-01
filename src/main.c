#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct wayWindow {
  SDL_Window *window;
  SDL_GPUDevice *GPUDevice;
  SDL_PropertiesID props;
} wayWindow;

int main(int argc, char *argv[]) {

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());

    return -1;
  }

  wayWindow wayWindow;

  wayWindow.GPUDevice =
      SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);

  if (wayWindow.GPUDevice == NULL) {
    SDL_Log("GPUCreateDevice failed %s", SDL_GetError());
    return -1;
  }

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

  if (!SDL_ClaimWindowForGPUDevice(wayWindow.GPUDevice, wayWindow.window)) {
    SDL_Log("GPUClaimWindow failed %s", SDL_GetError());
    return -1;
  }

  SDL_ReleaseWindowFromGPUDevice(wayWindow.GPUDevice, wayWindow.window);
  SDL_DestroyWindow(wayWindow.window);
  SDL_DestroyGPUDevice(wayWindow.GPUDevice);

  return 0;
}
