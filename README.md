

# LunaScape – Simple Video Player with SDL3, FFmpeg, and OpenGL
(An extremly simple KDE Plasma video player using `kdialog` for file selection)

This Video Player somehow functions on KDE-Plasa with an nvidia GPU.
Not working on this anymore, was just an experiment with ffmpeg. So probably will throw an error on AMD Gpu's.

## Dependencies

To build and run LunaScape, you need the following dependencies installed:

- **[FFmpeg](https://ffmpeg.org/)**
- **[SDL3](https://github.com/libsdl-org/SDL)**

---

##  Controls

| Key      | Action                                      |
|----------|---------------------------------------------|
| `R`      | Load another video                         |
| `Space`  | Pause video                                |
| `F`      | Toggle Fullscreen mode                     |
| `Esc` (Fullscreen) | Exit Fullscreen mode             |
| `Esc` (Windowed)   | Close the program                |
| `M`      | Mute Audio                                 |

---

## Performance Benchmarks

> **Frame render time** measured with `glBeginQuery`:

| Method                               | Render Time |
|--------------------------------------|------------|
| Only `glTexSubImage2D`               | ~5ms       |
| Using a Pixel Buffer Object (PBO)    | ~2ms       |
| Using two Pixel Buffer Objects (PBO) | ~0.5ms     |

---

**Note:** It utilizes `kdialog` for file selection.
