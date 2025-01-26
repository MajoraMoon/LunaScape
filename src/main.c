#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

static SDL_Window *window = NULL;
static bool running = true;

int main(int argc, char *argv[])
{

    SDL_SetAppMetadata("LunaScape", "0.1", NULL);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());

        return SDL_APP_FAILURE;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    if (props == 0)
    {
        SDL_Log("Unable to create properties: %s", SDL_GetError());

        return SDL_APP_FAILURE;
    }

    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "LunaScape");
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, SCR_WIDTH);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, SCR_HEIGHT);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WAYLAND_SURFACE_ROLE_CUSTOM_BOOLEAN, true);

    window = SDL_CreateWindowWithProperties(props);

    if (window == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initiate Window with custom wayland context: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();

        return SDL_APP_FAILURE;
    }

    while (running)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
            {
                running = false;
            }
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    SDL_Log("LunaScape ended successfully.\n");

    return 0;
}
