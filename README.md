# SDL3 - ffmpeg - nevermind - openGL

## Dependencies

Dependencies that need to be installed:
- **ffmpeg**
- **SDL3**

SDL3 still has some problems, see `CMakeLists.txt`. It needs to be directly linked for some GPU API functions to work for some reason.

## On Wayland/Plasma 6

Frame render time measured with `glBeginQuery`:

| Method                               | Render Time |
|--------------------------------------|------------|
| only with `glTexSubImage2D`         | ~5ms       |
| with a Pixel Buffer Object          | ~2ms       |
| with two Pixel Buffer Objects       | ~0.5ms     |

## Keys

| Key      | Action                                      |
|----------|---------------------------------------------|
| R        | Load another video                         |
| Space    | Pause video                                |
| F        | Enter Fullscreen mode / Leave Fullscreen mode |
| Escape (Fullscreen) | Leave Fullscreen mode          |
| Escape (Windowed)   | Close Program                  |

