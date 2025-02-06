#include "mediaLoader.h"

VideoContainer *init_video_container(const char *filepath) {

  // careful, heap! free the memory later.
  VideoContainer *video = (VideoContainer *)malloc(sizeof(VideoContainer));
  if (!video) {
    printf("VideoContainer - Memory allocation error.\n");

    return NULL;
  }

  video->pFormatCtx = NULL;
  video->pCodecCtx = NULL;
  video->videoStreamIndex = -1;

  // opens the video container. Puts information into pFormatCtx.
  if (avformat_open_input(&video->pFormatCtx, filepath, NULL, NULL) != 0) {
    printf("Could not open Videofile.\n");
    free(video);

    return NULL;
  }

  // find the stream information (I know these comments are amazing useful)
  if (avformat_find_stream_info(video->pFormatCtx, NULL) < 0) {
    printf("Could not find any stream-information.\n");
    free(video);

    return NULL;
  }

  // iterates through all streams and sets the video StreamIndex to the first
  // video stream found
  for (int i = 0; i < video->pFormatCtx->nb_streams; i++) {
    if (video->pFormatCtx->streams[i]->codecpar->codec_type ==
        AVMEDIA_TYPE_VIDEO) {
      video->videoStreamIndex = i;
      break;
    }
  }

  if (video->videoStreamIndex == -1) {
    printf("No video-stream found.\n");
    avformat_close_input(&video->pFormatCtx);
    free(video);

    return NULL;
  }

  // finds the right decoder of the video. ffmpeg supports extremly many codecs
  // see: "ffmpeg -codecs"
  video->pCodec = avcodec_find_decoder(
      video->pFormatCtx->streams[video->videoStreamIndex]->codecpar->codec_id);

  if (!video->pCodec) {
    printf("Unsupported codec.\n");
    avformat_close_input(&video->pFormatCtx);
    free(video);

    return NULL;
  }

  // creates an empty codec and fills it with information.
  video->pCodecCtx = avcodec_alloc_context3(video->pCodec);
  avcodec_parameters_to_context(
      video->pCodecCtx,
      video->pFormatCtx->streams[video->videoStreamIndex]->codecpar);

  if (avcodec_open2(video->pCodecCtx, video->pCodec, NULL) < 0) {
    printf("Could not open Codec.\n");
    avcodec_free_context(&video->pCodecCtx);
    avformat_close_input(&video->pFormatCtx);
    free(video);

    return NULL;
  }

  // AV_PIX_FMT_RGB24 look up others later
  video->sws_ctx =
      sws_getContext(video->pCodecCtx->width, video->pCodecCtx->height,
                     video->pCodecCtx->pix_fmt, video->pCodecCtx->width,
                     video->pCodecCtx->height, AV_PIX_FMT_RGB24,
                     SWS_FAST_BILINEAR, NULL, NULL, NULL);

  return video;
}

vFrame *init_video_frames(VideoContainer *video) {

  vFrame *videoFrame = (vFrame *)malloc(sizeof(vFrame));
  if (!videoFrame) {
    printf("VideoFrame - Memory allocation error.\n");
    return NULL;
  }

  videoFrame->frame = av_frame_alloc();
  videoFrame->frameYUV = av_frame_alloc();
  videoFrame->packet = av_packet_alloc();

  // reserves memory for the YUV-images and fills framYUV with the necessary
  // data
  int numBytes = av_image_get_buffer_size(
      AV_PIX_FMT_RGB24, video->pCodecCtx->width, video->pCodecCtx->height, 32);
  videoFrame->imgBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

  av_image_fill_arrays(videoFrame->frameYUV->data,
                       videoFrame->frameYUV->linesize, videoFrame->imgBuffer,
                       AV_PIX_FMT_RGB24, video->pCodecCtx->width,
                       video->pCodecCtx->height, 32);

  return videoFrame;
}

int video_container_get_frame(VideoContainer *video, vFrame *videoFrame) {

  while (video->paused) {
    SDL_Delay(10);
  }

  while (av_read_frame(video->pFormatCtx, videoFrame->packet) >= 0) {
    if (videoFrame->packet->stream_index == video->videoStreamIndex) {

      int send_status =
          avcodec_send_packet(video->pCodecCtx, videoFrame->packet);
      if (send_status < 0) {
        printf("Error sending packet: %d\n", send_status);
        av_packet_unref(videoFrame->packet);
        continue; // try with the next package.
      }

      int receive_status =
          avcodec_receive_frame(video->pCodecCtx, videoFrame->frame);
      if (receive_status == 0) { // if successfully received frames
        sws_scale(video->sws_ctx,
                  (const uint8_t *const *)videoFrame->frame->data,
                  videoFrame->frame->linesize, 0, video->pCodecCtx->height,
                  videoFrame->frameYUV->data, videoFrame->frameYUV->linesize);

        av_packet_unref(videoFrame->packet);
        return 1; // successfully decoded frame/frames
      } else if (receive_status == AVERROR(EAGAIN)) {
        printf("Decoder needs more data...\n");
        av_packet_unref(videoFrame->packet);
        continue; // wait for more data to decode
      } else {
        printf("Error receiving frame: %d\n", receive_status);
        av_packet_unref(videoFrame->packet);
        return 0; // something really baaaadd happened receiving frames.
      }
    }

    av_packet_unref(videoFrame->packet);
  }

  return 0;
}

void free_video_data(VideoContainer *video) {

  if (!video) {
    return;
  }

  sws_freeContext(video->sws_ctx);
  avcodec_free_context(&video->pCodecCtx);
  avformat_close_input(&video->pFormatCtx);
  free(video);
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
