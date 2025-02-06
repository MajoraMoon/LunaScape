SDL3 - ffmpeg - nevermind -openGL. 

On Wayland/Plasma 6

Frame Render time measured with "glBeginQuery":

only with "glTexSubImage2D" :  ~5ms

with a Pixel Buffer Object:    ~2ms

with two Pixel Buffer Objects: ~0.5ms
