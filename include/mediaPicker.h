#ifndef MEDIA_PICKER_H
#define MEDIA_PICKER_H

// clang-format off
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>


#include "mediaLoader.h"
#include "audioManager.h"


char *KDE_Plasma_select_video_file(void);

bool reload_video_and_audio(VideoContainer **video, vFrame **videoFrame,
    AudioManager *audioManager, uint64_t *start_time);

#endif