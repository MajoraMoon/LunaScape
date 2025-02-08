#include "audioManager.h"

int portaudio_init(PaStream **stream, unsigned int sample_rate, int channels,
                   PaStreamCallback *callback, void *userData) {

  PaError paErr = Pa_Initialize();

  if (paErr != paNoError) {
    fprintf(stderr, "PortAudio initialization error: %s\n",
            Pa_GetErrorText(paErr));

    return -1;
  }

  paErr =
      Pa_OpenDefaultStream(stream, 0, channels, paFloat32, sample_rate,
                           paFramesPerBufferUnspecified, callback, userData);

  if (paErr != paNoError) {
    fprintf(stderr, "PortAudio open stream error: %s\n",
            Pa_GetErrorText(paErr));
    Pa_Terminate();
    return -1;
  }
  paErr = Pa_StartStream(*stream);
  if (paErr != paNoError) {
    fprintf(stderr, "PortAudio start stream error: %s\n",
            Pa_GetErrorText(paErr));
    Pa_CloseStream(*stream);
    Pa_Terminate();
    return -1;
  }
  return 0;
}