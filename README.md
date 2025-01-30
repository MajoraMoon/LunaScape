Set wayland as the primary sdl video driver for all users on the System with:

"echo 'SDL_VIDEODRIVER=wayland' | sudo tee -a /etc/environment"


information:

    SDL on pure wayland needs a renderer to be active or it won't display the window correctly.