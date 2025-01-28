/**
 *
 * "<libavformat/avformat.h>" opens,reads and writes media in multiple formats.
 *
 *  "<libavcodec/avcodec.h>" provides the codecs for audio/video.
 *
 *  "<libswscale/swscale.h>" converts "pixel-formats". SDL uses YUV to improve performance on gpu rendering if I understood it correctly.
 */
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#include <wayWindow.h>

#define VIDEO_FILE "../video.mp4" // actual video to use

int main(int argc, char *argv[])
{

    // ffmpeg configuration

    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pCodecCtx = NULL;
    const AVCodec *pCodec = NULL;
    int videoStreamIndex = -1;

    // open the video
    if (avformat_open_input(&pFormatCtx, VIDEO_FILE, NULL, NULL) != 0)
    {
        printf("Could not open Videofile.\n");

        return -1;
    }

    // find the stream information (I know these comments are amazing useful)
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("Could not find any stream-information.\n");

        return -1;
    }

    for (int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1)
    {
        printf("No video-stream found.\n");

        return -1;
    }

    pCodec = avcodec_find_decoder(pFormatCtx->streams[videoStreamIndex]->codecpar->codec_id);

    if (pCodec == NULL)
    {
        printf("Could not find a fitting Codec.\n");

        return -1;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStreamIndex]->codecpar);

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        printf("Could not open Codec.\n");

        return -1;
    }

    // sdl configuration

    SDL_Window *window = initWayWindow("LunaScape", "0.1", 1280, 720, true);

    if (window == NULL)
    {
        SDL_Log("Something went wrong in setting up a SDL window.\n");
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        SDL_Log("Unable to create renderer %s\n", SDL_GetError());
        return -1;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *frameYUV = av_frame_alloc();

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 32);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

    av_image_fill_arrays(frameYUV->data, frameYUV->linesize, buffer,
                         AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 32);

    struct SwsContext *sws_ctx = sws_getContext(
        pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
        pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, NULL, NULL, NULL);

    bool running = true;
    while (running)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
            {
                running = false;
            }
        }

        // Lies ein Frame
        if (av_read_frame(pFormatCtx, packet) >= 0)
        {
            if (packet->stream_index == videoStreamIndex)
            {
                if (avcodec_send_packet(pCodecCtx, packet) == 0)
                {
                    while (avcodec_receive_frame(pCodecCtx, frame) == 0)
                    {
                        // Konvertiere das Bild in YUV
                        sws_scale(sws_ctx, (const uint8_t *const *)frame->data,
                                  frame->linesize, 0, pCodecCtx->height,
                                  frameYUV->data, frameYUV->linesize);

                        // Kopiere das Bild in die Textur
                        SDL_UpdateYUVTexture(texture, NULL,
                                             frameYUV->data[0], frameYUV->linesize[0],
                                             frameYUV->data[1], frameYUV->linesize[1],
                                             frameYUV->data[2], frameYUV->linesize[2]);

                        // Rendern
                        SDL_RenderClear(renderer);
                        SDL_RenderTexture(renderer, texture, NULL, NULL);
                        SDL_RenderPresent(renderer);
                    }
                }
            }
            av_packet_unref(packet);
        }
        else
        {
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
