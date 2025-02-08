#include "mediaLoader.h"

/**
 *                                                                    |
 *                                                                    |
 * ----------------- VIDEO DECODING FUNCTIONS START ------------------|
 *                                                                    |
 *                                                                    |
 */

// hardware decoding format when detected
static enum AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;

// Callback zum Auswählen des richtigen Hardware-Pixel-Formats
enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                 const enum AVPixelFormat *pix_fmts) {
  const enum AVPixelFormat *p;
  for (p = pix_fmts; *p != -1; p++) {
    if (*p == hw_pix_fmt) {
      return *p;
    }
  }
  fprintf(stderr, "No righ HW-Pixel-Format found.\n");
  return AV_PIX_FMT_NONE;
}

VideoContainer *init_video_container(const char *filepath,
                                     bool force_software) {

  // careful, heap! free the memory later.
  VideoContainer *video = (VideoContainer *)malloc(sizeof(VideoContainer));
  if (!video) {
    printf("VideoContainer - Memory allocation error.\n");
    return NULL;
  }

  video->pFormatCtx = NULL;
  video->pCodecCtx = NULL;
  video->videoStreamIndex = -1;
  video->hw_device_ctx = NULL;
  video->hw_sws_ctx = NULL;
  video->paused = false;

  // opens the video container. Puts information into pFormatCtx.
  if (avformat_open_input(&video->pFormatCtx, filepath, NULL, NULL) != 0) {
    printf("Could not open Videofile.\n");
    free(video);
    return NULL;
  }

  // find the stream information (I know these comments are amazing useful)
  if (avformat_find_stream_info(video->pFormatCtx, NULL) < 0) {
    printf("Could not find any stream-information.\n");
    avformat_close_input(&video->pFormatCtx);
    free(video);
    return NULL;
  }

  // av_dump_format(video->pFormatCtx, 0, filepath, 0);

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

  // finds the right decoder of the video. ffmpeg supports extremely many codecs
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

  // Hardware decoding setup start

  // setup decods
  if (!force_software) {
    // Hardware decoding setup
    enum AVHWDeviceType type = av_hwdevice_find_type_by_name("cuda");
    if (type != AV_HWDEVICE_TYPE_NONE) {
      hw_pix_fmt = AV_PIX_FMT_CUDA;
      printf("Using CUDA for hardware decoding.\n");
    } else {
      type = av_hwdevice_find_type_by_name("vaapi");
      if (type != AV_HWDEVICE_TYPE_NONE) {
        hw_pix_fmt = AV_PIX_FMT_VAAPI;
        printf("Using VAAPI for hardware decoding.\n");
      }
    }

    if (type != AV_HWDEVICE_TYPE_NONE) {
      int err =
          av_hwdevice_ctx_create(&video->hw_device_ctx, type, NULL, NULL, 0);
      if (err < 0) {
        fprintf(stderr, "Failed to create HW device context (err=%d)\n", err);
      } else {
        video->pCodecCtx->hw_device_ctx = av_buffer_ref(video->hw_device_ctx);
        video->pCodecCtx->get_format = get_hw_format;
      }
    }
  }
  // Hardware decoding setup end

  if (avcodec_open2(video->pCodecCtx, video->pCodec, NULL) < 0) {
    printf("Could not open Codec.\n");
    if (video->hw_device_ctx)
      av_buffer_unref(&video->hw_device_ctx);
    avcodec_free_context(&video->pCodecCtx);
    avformat_close_input(&video->pFormatCtx);
    free(video);
    return NULL;
  }

  // AV_PIX_FMT_RGB24 look up others later
  // This sws_ctx is used for software-decoded Frames
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

  // reserves memory for the YUV-images and fills frameYUV with the necessary
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
  // Reads packages, until a frame could be  successfully decoded.
  while (av_read_frame(video->pFormatCtx, videoFrame->packet) >= 0) {
    if (videoFrame->packet->stream_index == video->videoStreamIndex) {

      int send_status =
          avcodec_send_packet(video->pCodecCtx, videoFrame->packet);
      if (send_status < 0) {
        printf("Error sending packet: %d\n", send_status);
        av_packet_unref(videoFrame->packet);
        continue; // try next package
      }

      int receive_status;
      while ((receive_status = avcodec_receive_frame(video->pCodecCtx,
                                                     videoFrame->frame)) == 0) {

        // Use hardware decoding if frames are in the right format
        if (video->hw_device_ctx && videoFrame->frame->format == hw_pix_fmt) {
          AVFrame *sw_frame = av_frame_alloc();
          if (!sw_frame) {
            printf("Could not allocate data for hardware decoded frames.\n");
            av_packet_unref(videoFrame->packet);
            return 0;
          }

          if (av_hwframe_transfer_data(sw_frame, videoFrame->frame, 0) < 0) {
            printf("Error transferring frame from GPU to system memory.\n");
            av_frame_free(&sw_frame);
            av_packet_unref(videoFrame->packet);
            return 0;
          }

          if (!video->hw_sws_ctx) {

            video->hw_sws_ctx = sws_getContext(
                video->pCodecCtx->width, video->pCodecCtx->height,
                sw_frame->format, video->pCodecCtx->width,
                video->pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR,
                NULL, NULL, NULL);
            if (!video->hw_sws_ctx) {
              printf("Failed to create sws context for hardware decoded frame "
                     "conversion.\n");
              av_frame_free(&sw_frame);
              av_packet_unref(videoFrame->packet);
              return 0;
            }
          }

          sws_scale(video->hw_sws_ctx, (const uint8_t *const *)sw_frame->data,
                    sw_frame->linesize, 0, video->pCodecCtx->height,
                    videoFrame->frameYUV->data, videoFrame->frameYUV->linesize);
          av_frame_free(&sw_frame);
        } else {
          // Software-Decoding: use already existing sws_ctx
          sws_scale(video->sws_ctx,
                    (const uint8_t *const *)videoFrame->frame->data,
                    videoFrame->frame->linesize, 0, video->pCodecCtx->height,
                    videoFrame->frameYUV->data, videoFrame->frameYUV->linesize);
        }

        av_packet_unref(videoFrame->packet);
        return 1; // frame decoded successfully. This fuction should always
                  // return 1.
      }

      if (receive_status == AVERROR(EAGAIN)) {
        // There are no frames in htis package avaiable anymore. Read the next
        // package
        av_packet_unref(videoFrame->packet);
        continue;
      } else if (receive_status != AVERROR_EOF) {
        printf("Error receiving frame: %d\n", receive_status);
        av_packet_unref(videoFrame->packet);
        return 0;
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
  if (video->hw_sws_ctx) {
    sws_freeContext(video->hw_sws_ctx);
  }
  avcodec_free_context(&video->pCodecCtx);
  if (video->hw_device_ctx) {
    av_buffer_unref(&video->hw_device_ctx);
  }
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

/**
 *                                                                  |
 *                                                                  |
 * ----------------- VIDEO DECODING FUNCTIONS END ----------------- |
 *                                                                  |
 *                                                                  |
 */

/**
 *                                                                   |
 *                                                                   |
 * ----------------- AUDIO DECODING FUNCTIONS START----------------- |
 *                                                                   |
 *                                                                   |
 */

AudioContainer *init_audio_container(const char *filepath) {
  AudioContainer *audio = (AudioContainer *)malloc(sizeof(AudioContainer));
  if (!audio) {
    fprintf(stderr, "AudioContainer - Memory allocation error.\n");
    return NULL;
  }

  audio->pFormatCtx = NULL;
  audio->pCodecCtx = NULL;
  audio->pCodec = NULL;
  audio->audioStreamIndex = -1;
  audio->paused = false;
  audio->swr_ctx = NULL;

  // Open the audio file.
  if (avformat_open_input(&audio->pFormatCtx, filepath, NULL, NULL) != 0) {
    fprintf(stderr, "Could not open audio file.\n");
    free(audio);
    return NULL;
  }

  if (avformat_find_stream_info(audio->pFormatCtx, NULL) < 0) {
    fprintf(stderr, "Could not find stream information.\n");
    avformat_close_input(&audio->pFormatCtx);
    free(audio);
    return NULL;
  }

  // Locate the first audio stream.
  for (unsigned int i = 0; i < audio->pFormatCtx->nb_streams; i++) {
    if (audio->pFormatCtx->streams[i]->codecpar->codec_type ==
        AVMEDIA_TYPE_AUDIO) {
      audio->audioStreamIndex = i;
      break;
    }
  }
  if (audio->audioStreamIndex == -1) {
    fprintf(stderr, "No audio stream found.\n");
    avformat_close_input(&audio->pFormatCtx);
    free(audio);
    return NULL;
  }

  // Find and open the decoder.
  audio->pCodec = avcodec_find_decoder(
      audio->pFormatCtx->streams[audio->audioStreamIndex]->codecpar->codec_id);
  if (!audio->pCodec) {
    fprintf(stderr, "Unsupported audio codec.\n");
    avformat_close_input(&audio->pFormatCtx);
    free(audio);
    return NULL;
  }

  audio->pCodecCtx = avcodec_alloc_context3(audio->pCodec);
  if (!audio->pCodecCtx) {
    fprintf(stderr, "Could not allocate audio codec context.\n");
    avformat_close_input(&audio->pFormatCtx);
    free(audio);
    return NULL;
  }

  if (avcodec_parameters_to_context(
          audio->pCodecCtx,
          audio->pFormatCtx->streams[audio->audioStreamIndex]->codecpar) < 0) {
    fprintf(stderr, "Failed to copy audio codec parameters.\n");
    avcodec_free_context(&audio->pCodecCtx);
    avformat_close_input(&audio->pFormatCtx);
    free(audio);
    return NULL;
  }

  if (avcodec_open2(audio->pCodecCtx, audio->pCodec, NULL) < 0) {
    fprintf(stderr, "Could not open audio codec.\n");
    avcodec_free_context(&audio->pCodecCtx);
    avformat_close_input(&audio->pFormatCtx);
    free(audio);
    return NULL;
  }

  // Allocate the resampler context.
  audio->swr_ctx = swr_alloc();
  if (!audio->swr_ctx) {
    fprintf(stderr, "Could not allocate resampler context.\n");
    avcodec_free_context(&audio->pCodecCtx);
    avformat_close_input(&audio->pFormatCtx);
    free(audio);
    return NULL;
  }

  // Prepare output channel layout (stereo) using AVChannelLayout.
  AVChannelLayout out_ch_layout;
  av_channel_layout_default(&out_ch_layout, 2); // 2 channels for stereo.
  enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_FLT;
  int out_sample_rate = audio->pCodecCtx->sample_rate;

  // Prepare input channel layout.
  AVChannelLayout in_ch_layout;
  if (audio->pCodecCtx->ch_layout.nb_channels > 0) {
    in_ch_layout = audio->pCodecCtx->ch_layout;
  } else if (audio->pFormatCtx->streams[audio->audioStreamIndex]
                 ->codecpar->ch_layout.nb_channels > 0) {
    in_ch_layout = audio->pFormatCtx->streams[audio->audioStreamIndex]
                       ->codecpar->ch_layout;
  } else {
    // As a last resort, default to stereo.
    av_channel_layout_default(&in_ch_layout, 2);
  }

  // Set options for the resampler.
  // swr_alloc_set_opts2 now expects 9 arguments.
  if (swr_alloc_set_opts2(&audio->swr_ctx, &out_ch_layout, out_sample_fmt,
                          out_sample_rate, &in_ch_layout,
                          audio->pCodecCtx->sample_fmt,
                          audio->pCodecCtx->sample_rate, 0, NULL) < 0) {
    fprintf(stderr, "Failed to set options for the resampling context.\n");
    swr_free(&audio->swr_ctx);
    avcodec_free_context(&audio->pCodecCtx);
    avformat_close_input(&audio->pFormatCtx);
    free(audio);
    return NULL;
  }

  if (swr_init(audio->swr_ctx) < 0) {
    fprintf(stderr, "Failed to initialize the resampling context.\n");
    swr_free(&audio->swr_ctx);
    avcodec_free_context(&audio->pCodecCtx);
    avformat_close_input(&audio->pFormatCtx);
    free(audio);
    return NULL;
  }

  return audio;
}

aFrame *init_audio_frames(AudioContainer *audio) {
  (void)audio; // Unused parameter.
  aFrame *audioFrame = (aFrame *)malloc(sizeof(aFrame));
  if (!audioFrame) {
    fprintf(stderr, "aFrame - Memory allocation error.\n");
    return NULL;
  }
  audioFrame->frame = av_frame_alloc();
  audioFrame->packet = av_packet_alloc();
  audioFrame->convertedData = NULL;
  audioFrame->convertedDataSize = 0;
  return audioFrame;
}

int audio_container_get_frame(AudioContainer *audio, aFrame *audioFrame) {
  // If paused, wait.
  while (audio->paused) {
    SDL_Delay(10);
  }

  // Read packets until an audio frame is decoded.
  while (av_read_frame(audio->pFormatCtx, audioFrame->packet) >= 0) {
    if (audioFrame->packet->stream_index == audio->audioStreamIndex) {
      int ret = avcodec_send_packet(audio->pCodecCtx, audioFrame->packet);
      if (ret < 0) {
        fprintf(stderr, "Error sending audio packet: %d\n", ret);
        av_packet_unref(audioFrame->packet);
        continue;
      }
      ret = avcodec_receive_frame(audio->pCodecCtx, audioFrame->frame);
      if (ret == 0) {
        if (audioFrame->convertedData) {
          av_freep(&audioFrame->convertedData[0]);
          free(audioFrame->convertedData);
          audioFrame->convertedData = NULL;
        }
        int dst_nb_samples = av_rescale_rnd(
            swr_get_delay(audio->swr_ctx, audio->pCodecCtx->sample_rate) +
                audioFrame->frame->nb_samples,
            audio->pCodecCtx->sample_rate, audio->pCodecCtx->sample_rate,
            AV_ROUND_UP);

        int ret_alloc = av_samples_alloc_array_and_samples(
            &audioFrame->convertedData, NULL,
            2, // output channels (stereo)
            dst_nb_samples, AV_SAMPLE_FMT_FLT, 0);
        if (ret_alloc < 0) {
          fprintf(stderr, "Could not allocate converted samples buffer.\n");
          av_packet_unref(audioFrame->packet);
          return 0;
        }

        int nb_converted = swr_convert(
            audio->swr_ctx, audioFrame->convertedData, dst_nb_samples,
            (const uint8_t **)audioFrame->frame->data,
            audioFrame->frame->nb_samples);
        if (nb_converted < 0) {
          fprintf(stderr, "Error while converting audio samples.\n");
          av_packet_unref(audioFrame->packet);
          return 0;
        }

        int size = av_samples_get_buffer_size(NULL, 2, nb_converted,
                                              AV_SAMPLE_FMT_FLT, 1);
        audioFrame->convertedDataSize = size;

        av_packet_unref(audioFrame->packet);
        return 1;
      } else if (ret == AVERROR(EAGAIN)) {
        av_packet_unref(audioFrame->packet);
        continue;
      } else if (ret == AVERROR_EOF) {
        av_packet_unref(audioFrame->packet);
        return 0;
      } else {
        fprintf(stderr, "Error receiving audio frame: %d\n", ret);
        av_packet_unref(audioFrame->packet);
        return 0;
      }
    }
    av_packet_unref(audioFrame->packet);
  }
  return 0;
}

void free_audio_data(AudioContainer *audio) {
  if (!audio)
    return;
  if (audio->swr_ctx)
    swr_free(&audio->swr_ctx);
  if (audio->pCodecCtx)
    avcodec_free_context(&audio->pCodecCtx);
  if (audio->pFormatCtx)
    avformat_close_input(&audio->pFormatCtx);
  free(audio);
}

void free_audio_frames(aFrame *audioFrame) {
  if (!audioFrame)
    return;
  if (audioFrame->frame)
    av_frame_free(&audioFrame->frame);
  if (audioFrame->packet)
    av_packet_free(&audioFrame->packet);
  if (audioFrame->convertedData) {
    av_freep(&audioFrame->convertedData[0]);
    free(audioFrame->convertedData);
  }
  free(audioFrame);
}

/**
 *                                                                 |
 *                                                                 |
 * ----------------- AUDIO DECODING FUNCTIONS END----------------- |
 *                                                                 |
 *                                                                 |
 */