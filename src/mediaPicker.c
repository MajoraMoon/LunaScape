
#include <mediaPicker.h>

// relativ path to absolute path. Goes on path above the one, where the
// programm
// was executed.
static char *get_project_root() {
  static char path[PATH_MAX];
  if (realpath("..", path)) {
    return path;
  }
  return NULL;
}

// implementation on KDE Plasma , might add more later
char *KDE_Plasma_select_video_file(void) {
  char buffer[1024] = {0};
  char command[1200];

  char *project_root = get_project_root();
  if (!project_root) {
    SDL_Log("Could not define project root.");
    return NULL;
  }

  // to open kdialog on KDE plasma
  snprintf(command, sizeof(command),
           "kdialog --getopenfilename \"%s\" \"Videos (*.mp4 *.mkv *.avi *.mov "
           "*.webm *.flv)\"",
           project_root);

  // opens the shell with the command above
  FILE *fp = popen(command, "r");
  if (!fp) {
    SDL_Log("Error: popen() failed.");
    return NULL;
  }

  if (!fgets(buffer, sizeof(buffer), fp)) {
    SDL_Log("Error: No file selected.");
    pclose(fp);
    return NULL;
  }
  pclose(fp);

  buffer[strcspn(buffer, "\n")] = '\0';

  if (strlen(buffer) == 0) {
    SDL_Log("Error: No supported files selected.");
    return NULL;
  }

  char *filename = malloc(strlen(buffer) + 1);
  if (filename)
    strcpy(filename, buffer);

  return filename;
}

/**
 * returns true if new file was selected.
 * Returns false when either no file was selected or the video/videoFrame setup
 * failed.
 */
bool reload_video(VideoContainer **video, vFrame **videoFrame,
                  uint64_t *start_time) {

  // Select new file first
  char *new_video_file = KDE_Plasma_select_video_file();
  if (!new_video_file || new_video_file[0] == '\0') {

    if (new_video_file) {
      free(new_video_file);
    }

    return false;
  }

  if (*video) {
    free_video_data(*video);

    *video = NULL;
  }

  if (*videoFrame) {
    free_video_frames(*videoFrame);

    *videoFrame = NULL;
  }

  *video = init_video_container(new_video_file, false);

  if (!*video) {
    SDL_Log("Failed to initialize new video container.");
    free(new_video_file);
    return false;
  }

  *videoFrame = init_video_frames(*video);
  if (!*videoFrame) {
    SDL_Log("Failed to initialize new video frames.");
    free_video_data(*video);
    *video = NULL;
    free(new_video_file);
    return false;
  }

  *start_time = SDL_GetTicksNS();

  free(new_video_file);

  return true;
}