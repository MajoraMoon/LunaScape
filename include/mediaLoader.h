#ifndef MEDIA_LOADER_H
#define MEDIA_LOADER_H

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

/**
 *
 * "<libavformat/avformat.h>" opens,reads and writes media in multiple formats.
 *
 *  "<libavcodec/avcodec.h>" provides the codecs for audio/video.
 *
 *  "<libswscale/swscale.h>" converts "pixel-formats". SDL uses YUV to improve
 * performance on gpu rendering if I understood it correctly.
 *
 *  "include <libavutil/imgutils.h>" provides extra functions for calculating
 * memory for given frames. In simple terms, just learning lol.
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include <libavutil/channel_layout.h> // Audio: channellayouts
#include <libavutil/samplefmt.h>      // Audio: Sample-Formats
#include <libswresample/swresample.h> // Audio: Resampling

/**
 * General information:
 *
 *  A video file is basically a container for various data. This data must be
 *  handled separately. This data is called "stream". So a flow of information
 *  which is updated all stream long.
 *  There are one (or multiply) video-streams, audio-streams and if available
 * subtitle-data.
 *
 *
 *  When someone is watching a movie on an officle (or unofficial) streaming
 *  service, they can often choose between multiple languages and subtitles.
 *  This can be achieved by using a container-format which supports these
 * features.
 *
 *  A so called MP4 (MPEG-4 Part 14) - format was designed for streaming videos
 * and having a high compatibility with hardware. This format only supports one
 * video stream and sometimes multiple audio streams (which is rarely used).
 * Subtitles are also supported for a limited extend.
 *
 *  A so called MKV (Matroska) - format is a way more flexible and open source
 *  form for storing multiple video, audio, subtitle -streams.
 *  Chapters and other informations can be easily stored too.
 *  This is often used for modern streaming services.
 *
 */

/**
 *
 * Decoder:
 *
 * Video information is basically just an iteration of an array filled with
 * images. Depending on the quality of the image, it could contain a lot of
 * information and thererfore it can have a big filesize. Raw video information
 * can be really big. For that reason there are algorithms for compressing
 * various information. There are many so called "Codecs" which are responsible
 * for the compression. known ones are H.264, VP9 or AV1.
 *
 * A Decoder is something that will take the compress data and ...decodes it.
 * lol
 *
 */

/**
 *
 * Using YUV instead of RGB :
 *
 * Many video-codecs are using YUV instead of RGB for color and brightness
 * information. YUV can seperate brightness and color information, then these
 * values can be reduced in a better way. So the final video will be reduced in
 * Quality but it won't be really visible. That can reduce the cost of decoding
 * (So I assume that).
 */

// ffmpeg configuration

/**
 * pFormatCtx contains general information about the file.argc
 * pCodecCtx is managing the video decoder
 * pCodec will be used for the actual decoder
 * videoStreamIndex contains the index of the first video stream
 * a swsContext for converting RGB data to YUV420P data
 */

typedef struct VideoContainer {

  AVFormatContext *pFormatCtx;
  AVCodecContext *pCodecCtx;
  AVBufferRef *hw_device_ctx;

  const AVCodec *pCodec;
  int videoStreamIndex;
  bool paused;
  struct SwsContext *sws_ctx;
  struct SwsContext *hw_sws_ctx; // sws codec for Hardware-Decoding

} VideoContainer;

/**
 *
 * *packet saves all the compressed information (frames)
 * *frame saves all decoded frames
 * *frameYUV is for the YUV converted frames
 *
 */

typedef struct vFrame {
  AVFrame *frame;
  AVFrame *frameYUV;
  AVPacket *packet;
  uint8_t *imgBuffer;
} vFrame;

// Same structs for audio decoding

typedef struct AudioContainer {
  AVFormatContext *pFormatCtx;
  AVCodecContext *pCodecCtx;

  const AVCodec *pCodec;
  int audioStreamIndex;
  bool paused;
  struct SwrContext *swr_ctx;
} AudioContainer;

typedef struct aFrame {
  AVFrame *frame;
  AVPacket *packet;
  uint8_t **convertedData;
  int convertedDataSize;
} aFrame;

// VIDEO DECODING FUNCTIONS
enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                 const enum AVPixelFormat *pix_fmts);

VideoContainer *init_video_container(const char *filepath, bool force_software);

void free_video_data(VideoContainer *video);

vFrame *init_video_frames(VideoContainer *video);

void free_video_frames(vFrame *videoFrame);

int video_container_get_frame(VideoContainer *video, vFrame *videoFrame);

// AUDIO DECODING FUNCTIONS

AudioContainer *init_audio_container(const char *filepath);
void free_audio_data(AudioContainer *audio);
aFrame *init_audio_frames(AudioContainer *audio);
void free_audio_frames(aFrame *audioFrame);
int audio_container_get_frame(AudioContainer *audio, aFrame *audioFrame);

#endif