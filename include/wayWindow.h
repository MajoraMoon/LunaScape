#ifndef WAYWINDOW_H

#define WAYWINDOW_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

// initiate a SDL3 window with "SDL_PROP_WINDOW_CREATE_WAYLAND_SURFACE_ROLE_CUSTOM_BOOLEAN" property.
// It's for using external wayland libraries, at least I undrstood it that way.
SDL_Window *initWayWindow(const char *title, const char *version, unsigned int width, unsigned int height, bool resizableWindow);

void cleanupWindow(SDL_Window *window);

#endif