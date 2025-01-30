#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include <stdio.h>
#include <stdlib.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

typedef struct VideoPlayer {
  AVFormatContext *pFormatCtx;
  AVCodecContext *pCodecCtx;
  const AVCodec *pCodec;
  int videoStreamIndex;
  struct SwsContext *sws_ctx;

} VideoPlayer;

typedef struct vFrame {
  AVFrame *frame;
  AVFrame *frameYUV;
  AVPacket *packet;
  uint8_t *imgBuffer;
} vFrame;

VideoPlayer *init_video_player(const char *filepath);

void free_video_player(VideoPlayer *player);

vFrame *init_video_frames(VideoPlayer *player);
void free_video_frames(vFrame *videoFrame);

int video_player_get_frame(VideoPlayer *player, vFrame *videoFrame);

#endif
