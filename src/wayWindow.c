#include <wayWindow.h>

SDL_Window *initWayWindow(const char *title, const char *version,
                          unsigned int width, unsigned int height,
                          bool resizableWindow) {
  // Metadata is new in SDL3, why not using it :)
  SDL_SetAppMetadata(title, version, NULL);

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());

    return NULL;
  }

  // instead of creating a window with properties directly, using the more
  // modular approach here.
  SDL_PropertiesID props = SDL_CreateProperties();
  if (props == 0) {
    SDL_Log("Unable to create properties: %s", SDL_GetError());

    return NULL;
  }

  // set pCodecCtx->width and pCodecCtx->height for opening the window with the
  // dimensions of the videofile
  SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN,
                         resizableWindow);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);

  // later implementating a different wayland window session. If activated now,
  // no window will be displayed.
  //  SDL_SetNumberProperty(props,
  //  SDL_PROP_WINDOW_CREATE_WAYLAND_SURFACE_ROLE_CUSTOM_BOOLEAN, true);
  SDL_Window *window = SDL_CreateWindowWithProperties(props);

  if (window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Could not initiate Window with custom wayland context: %s\n",
                 SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();

    return NULL;
  } else {
    return window;
  }
}

void cleanupWindow(SDL_Window *window) {

  SDL_DestroyWindow(window);
  SDL_Quit();
  SDL_Log("LunaScape ended successfully.\n");
}