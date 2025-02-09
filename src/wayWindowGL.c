#include <wayWindowGL.h>

SDL_Window *initWayWindowGL(const char *title, const char *version,
                            unsigned int width, unsigned int height,
                            bool resizableWindow)
{

  // Metadata is new in SDL3, why not using it :)
  SDL_SetAppMetadata(title, version, NULL);

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
  {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());

    return NULL;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  // instead of creating a window with properties directly, using the more
  // modular approach here.
  SDL_PropertiesID props = SDL_CreateProperties();
  if (props == 0)
  {
    SDL_Log("Unable to create properties: %s", SDL_GetError());

    return NULL;
  }

  SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN,
                         resizableWindow);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN,
                         false);

  SDL_Window *window = SDL_CreateWindowWithProperties(props);

  if (window == NULL)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Could not initiate Window with custom wayland context: %s\n",
                 SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();

    return NULL;
  }

  return window;
}

SDL_GLContext initOpenGLContext_and_glad(SDL_Window *window)
{

  SDL_GLContext glContext = SDL_GL_CreateContext(window);

  if (!glContext)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Could not create OpenGL context: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return false;
  }

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
  {
    printf("Failed to initialize GLAD\n");
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return false;
  }

  return glContext;
}

void cleanupWindow(SDL_Window *window, SDL_GLContext glContext)
{

  SDL_GL_DestroyContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();
  SDL_Log("Quitted successfully");
}