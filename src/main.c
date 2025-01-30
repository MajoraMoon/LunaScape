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
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

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

#include <wayWindow.h>

#define VIDEO_FILE "../video.mp4" // actual video to use

int main(int argc, char *argv[]) {

  // ffmpeg configuration

  /**
   * pFormatCtx contains general information about the file.argc
   * pCodecCtx is managing the video decoder
   * pCodec will be used for the actual decoder
   * videoStreamIndex contains the index of the first video stream
   */

  AVFormatContext *pFormatCtx = NULL;
  AVCodecContext *pCodecCtx = NULL;
  const AVCodec *pCodec = NULL;
  int videoStreamIndex = -1;

  // opens the video container. Puts information into pFormatCtx.
  if (avformat_open_input(&pFormatCtx, VIDEO_FILE, NULL, NULL) != 0) {
    printf("Could not open Videofile.\n");

    return -1;
  }

  // find the stream information (I know these comments are amazing useful)
  if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
    printf("Could not find any stream-information.\n");

    return -1;
  }

  // iterates through all streams and sets the video StreamIndex to the first
  // video stream found
  for (int i = 0; i < pFormatCtx->nb_streams; i++) {
    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStreamIndex = i;
      break;
    }
  }

  if (videoStreamIndex == -1) {
    printf("No video-stream found.\n");

    return -1;
  }

  // finds the right decoder of the video. ffmpeg supports extremly many codecs
  // see: "ffmpeg -codecs"
  pCodec = avcodec_find_decoder(
      pFormatCtx->streams[videoStreamIndex]->codecpar->codec_id);

  if (pCodec == NULL) {
    printf("Could not find a fitting Codec.\n");

    return -1;
  }

  // creates an empty codec and fills it with information.
  pCodecCtx = avcodec_alloc_context3(pCodec);
  avcodec_parameters_to_context(
      pCodecCtx, pFormatCtx->streams[videoStreamIndex]->codecpar);

  if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
    printf("Could not open Codec.\n");

    return -1;
  }

  // sdl configuration
  // set pCodecCtx->width and pCodecCtx->height for opening the window with the
  // dimensions of the videofile
  SDL_Window *window = initWayWindow("LunaScape", "0.1", 1280, 720, true);

  if (window == NULL) {
    SDL_Log("Something went wrong in setting up a SDL window.\n");
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    SDL_Log("Unable to create renderer %s\n", SDL_GetError());
    return -1;
  }

  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           pCodecCtx->width, pCodecCtx->height);

  // SDL texture and ffmpeg information configuration

  /**
   *
   * *packet saves all the compressed information (frames)
   * *frame saves all decoded frames
   * *frameYUV is for the YUV converted frames
   */

  AVPacket *packet = av_packet_alloc();
  AVFrame *frame = av_frame_alloc();
  AVFrame *frameYUV = av_frame_alloc();

  // reserves memory for the YUV-images and fills framYUV with the necessary
  // data
  int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,
                                          pCodecCtx->height, 32);
  uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

  av_image_fill_arrays(frameYUV->data, frameYUV->linesize, buffer,
                       AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height,
                       32);

  // creates a swsContext for converting RGB data to YUV420P data
  struct SwsContext *sws_ctx = sws_getContext(
      pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width,
      pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);

  bool running = true;
  while (running) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
        running = false;
      }
    }

    // reads a single frame
    if (av_read_frame(pFormatCtx, packet) >= 0) {
      if (packet->stream_index == videoStreamIndex) {
        if (avcodec_send_packet(pCodecCtx, packet) == 0) {
          while (avcodec_receive_frame(pCodecCtx, frame) == 0) {
            // Converts a frame into the YUV format
            sws_scale(sws_ctx, (const uint8_t *const *)frame->data,
                      frame->linesize, 0, pCodecCtx->height, frameYUV->data,
                      frameYUV->linesize);

            // Copy a frame into a SDL texture
            SDL_UpdateYUVTexture(texture, NULL, frameYUV->data[0],
                                 frameYUV->linesize[0], frameYUV->data[1],
                                 frameYUV->linesize[1], frameYUV->data[2],
                                 frameYUV->linesize[2]);

            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
          }
        }
      }
      av_packet_unref(packet);
    } else {
      running = false;
    }
  }

  av_frame_free(&frame);
  av_frame_free(&frameYUV);
  av_packet_free(&packet);
  sws_freeContext(sws_ctx);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  avcodec_free_context(&pCodecCtx);
  avformat_close_input(&pFormatCtx);

  cleanupWindow(window);

  return 0;
}
