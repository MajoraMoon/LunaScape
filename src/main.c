#include <wayWindow.h>

int main(int argc, char *argv[])
{

    SDL_Window *window = initWayWindow("LunaScape", "0.1", 1280, 720, true);

    bool running = true;
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

    cleanupWindow(window);

    return 0;
}
