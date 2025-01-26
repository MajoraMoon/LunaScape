#ifndef WAYWINDOW_H

#define WAYWINDOW_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

SDL_Window *initWayWindow(const char *title, const char *version, unsigned int width, unsigned int height, bool resizableWindow);

void cleanupWindow(SDL_Window *window);

#endif