SDL3 - ffmpeg - nevermind -openGL. 

Dependencies that need to be installed: ffmpeg, SDL3.
SDL3 still has some problems, see CmakeList.txt. It needs to be directly linked for some GPU API functions to work for some reason.

On Wayland/Plasma 6

Frame Render time measured with "glBeginQuery":

only with "glTexSubImage2D" :  ~5ms

with a Pixel Buffer Object:    ~2ms

with two Pixel Buffer Objects: ~0.5ms
