This is a simple video player which uses ffmpeg and the intern 2d SLD renderer. 

It does not work correctly for some reason. SDL uses the wrong colorspace and looking into the SDL3 docs, there is no way to change that anymore.
So I will work on a version with the new SDL GPU api, ffmpeg and vulcan to render everything more efficient and better.



Set wayland as the primary sdl video driver for all users on the System with:

"echo 'SDL_VIDEODRIVER=wayland' | sudo tee -a /etc/environment"


information:

    SDL on pure wayland needs a renderer to be active or it won't display the window correctly.