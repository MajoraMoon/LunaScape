#include <video_player.h>

VideoPlayer *init_video_player(const char *filepath) {
  VideoPlayer *player = (VideoPlayer *)malloc(sizeof(VideoPlayer));
  if (!player) {
    printf("Video player - Memory allocation error.\n");
    return NULL;
  }

  player->pFormatCtx = NULL;
  player->pCodecCtx = NULL;
  player->videoStreamIndex = -1;

  // opens the video container. Puts information into pFormatCtx.
  if (avformat_open_input(&player->pFormatCtx, filepath, NULL, NULL) != 0) {
    printf("Could not open Videofile.\n");
    free(player);

    return NULL;
  }

  // find the stream information (I know these comments are amazing useful)
  if (avformat_find_stream_info(player->pFormatCtx, NULL) < 0) {
    printf("Could not find any stream-information.\n");
    free(player);
    return NULL;
  }

  // iterates through all streams and sets the video StreamIndex to the first
  // video stream found
  for (int i = 0; i < player->pFormatCtx->nb_streams; i++) {
    if (player->pFormatCtx->streams[i]->codecpar->codec_type ==
        AVMEDIA_TYPE_VIDEO) {
      player->videoStreamIndex = i;
      break;
    }
  }

  if (player->videoStreamIndex == -1) {
    printf("No video-stream found.\n");
    avformat_close_input(&player->pFormatCtx);
    free(player);

    return NULL;
  }

  // finds the right decoder of the video. ffmpeg supports extremly many codecs
  // see: "ffmpeg -codecs"
  player->pCodec =
      avcodec_find_decoder(player->pFormatCtx->streams[player->videoStreamIndex]
                               ->codecpar->codec_id);

  if (!player->pCodec) {
    printf("Unsupported codec.\n");
    avformat_close_input(&player->pFormatCtx);
    free(player);

    return NULL;
  }

  // creates an empty codec and fills it with information.
  player->pCodecCtx = avcodec_alloc_context3(player->pCodec);
  avcodec_parameters_to_context(
      player->pCodecCtx,
      player->pFormatCtx->streams[player->videoStreamIndex]->codecpar);

  if (avcodec_open2(player->pCodecCtx, player->pCodec, NULL) < 0) {
    printf("Could not open Codec.\n");
    avcodec_free_context(&player->pCodecCtx);
    avformat_close_input(&player->pFormatCtx);
    free(player);

    return NULL;
  }

  player->sws_ctx =
      sws_getContext(player->pCodecCtx->width, player->pCodecCtx->height,
                     player->pCodecCtx->pix_fmt, player->pCodecCtx->width,
                     player->pCodecCtx->height, AV_PIX_FMT_YUV420P,
                     SWS_BILINEAR, NULL, NULL, NULL);

  return player;
}

vFrame *init_video_frames(VideoPlayer *player) {

  vFrame *videoFrame = (vFrame *)malloc(sizeof(vFrame));
  if (!videoFrame) {
    printf("VideoFrame - Memory allocation error.\n");
    return NULL;
  }

  videoFrame->frame = av_frame_alloc();
  videoFrame->frameYUV = av_frame_alloc();
  videoFrame->packet = av_packet_alloc();

  int numBytes =
      av_image_get_buffer_size(AV_PIX_FMT_YUV420P, player->pCodecCtx->width,
                               player->pCodecCtx->height, 32);
  videoFrame->imgBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

  av_image_fill_arrays(videoFrame->frameYUV->data,
                       videoFrame->frameYUV->linesize, videoFrame->imgBuffer,
                       AV_PIX_FMT_YUV420P, player->pCodecCtx->width,
                       player->pCodecCtx->height, 32);

  return videoFrame;
}

int video_player_get_frame(VideoPlayer *player, vFrame *videoFrame) {

  if (av_read_frame(player->pFormatCtx, videoFrame->packet) < 0) {
    return 0;
  }

  if (videoFrame->packet->stream_index == player->videoStreamIndex) {
    if (avcodec_send_packet(player->pCodecCtx, videoFrame->packet) == 0) {
      if (avcodec_receive_frame(player->pCodecCtx, videoFrame->frame) == 0) {

        sws_scale(player->sws_ctx,
                  (const uint8_t *const *)videoFrame->frame->data,
                  videoFrame->frame->linesize, 0, player->pCodecCtx->height,
                  videoFrame->frameYUV->data, videoFrame->frameYUV->linesize);

        av_packet_unref(videoFrame->packet);
        return 1;
      }
    }
  }
  av_packet_unref(videoFrame->packet);
  return 0;
}

void free_video_player(VideoPlayer *player) {
  if (!player) {
    return;
  }
  sws_freeContext(player->sws_ctx);
  avcodec_free_context(&player->pCodecCtx);
  avformat_close_input(&player->pFormatCtx);
  free(player);
}

void free_video_frames(vFrame *videoFrame) {

  if (!videoFrame) {
    return;
  }

  av_frame_free(&videoFrame->frame);
  av_frame_free(&videoFrame->frameYUV);
  av_packet_free(&videoFrame->packet);
  av_free(videoFrame->imgBuffer);
  free(videoFrame);
}
