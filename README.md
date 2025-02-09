

# LunaScape – Simple Video Player with SDL3, FFmpeg, and OpenGL
(A KDE Plasma-optimized video player using `kdialog` for file selection)

## Dependencies

To build and run LunaScape, you need the following dependencies installed:

- **[FFmpeg](https://ffmpeg.org/)**
- **[SDL3](https://github.com/libsdl-org/SDL)**


## ⚙️ SDL3 Dependency

> ⚠ **Important:**  
> Direct linking is required for SDL3 because using `SDL3::SDL3` in `target_link_libraries`  
> results in undefined references for GPU API functions (e.g., `SDL_WaitAndAcquireGPUSwapchainTexture`).  
> Therefore, we manually link the libraries as shown below:

```cmake
find_package(SDL3 REQUIRED)
target_link_libraries(LunaScape PRIVATE 
    "/usr/lib/libSDL3.so"
    "/usr/local/lib/libSDL3_image.so"
)
```

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

## 📊 Performance Benchmarks

> **Frame render time** measured with `glBeginQuery`:

| Method                               | Render Time |
|--------------------------------------|------------|
| Only `glTexSubImage2D`               | ~5ms       |
| Using a Pixel Buffer Object (PBO)    | ~2ms       |
| Using two Pixel Buffer Objects (PBO) | ~0.5ms     |

---

💡 **Note:** This project is optimized for Plasma (Wayland).  
It utilizes `kdialog` for file selection.