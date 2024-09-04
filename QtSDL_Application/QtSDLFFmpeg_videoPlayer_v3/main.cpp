#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_thread.h>

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QString>


#include <mutex>
#include <condition_variable>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
// #include <iostream>
// #include <thread>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
}

// #include <queue>

#undef main

#include <QQueue>
#define FF_REFRESH_EVENT (SDL_USEREVENT)
#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)
#define FF_QUIT_EVENT (SDL_USEREVENT + 1)
#define VIDEO_PICTURE_QUEUE_SIZE 1
#define SDL_AUDIO_BUFFER_SIZE 1024

const int AVCODEC_MAX_AUDIO_FRAME_SIZE = 192000;

struct userData {
    Uint8 * audio_buf;
    Uint32 audio_len;
    int curr_len;
};

typedef struct Packet {
    AVPacket av_packet;
    struct Packet* next;
    int serial;
} Packet;

struct PacketQueue {
    Packet * first_pkt,*last_pkt;
    int nb_packets;
    int size;
    SDL_mutex * mutex;
    SDL_cond * cond;
};

typedef struct VideoPicture
{
    AVFrame *   frame;
    int         width;
    int         height;
    int         allocated;
} VideoPicture;

struct VideoState{
    AVFormatContext *   pFormatCtx;
    SDL_Window *        window = nullptr;
    int                 audioStream;
    AVStream *          audio_st;
    AVCodecContext *    audio_ctx;
    PacketQueue         audioq;
    uint8_t             audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) /2];
    unsigned int        audio_buf_size;
    unsigned int        audio_buf_index;
    AVFrame             audio_frame;
    AVPacket            audio_pkt;
    uint8_t *           audio_pkt_data;
    int                 audio_pkt_size;

    int                 videoStream;
    AVStream *          video_st;
    AVCodecContext *    video_ctx;
    SDL_Texture *       texture;
    SDL_Renderer *      renderer;
    PacketQueue         videoq;
    struct SwsContext * sws_ctx;
    VideoPicture        pictq[VIDEO_PICTURE_QUEUE_SIZE];
    int                 pictq_size;
    int                 pictq_rindex;
    int                 pictq_windex;
    SDL_mutex *         pictq_mutex;
    SDL_cond *          pictq_cond;

    SDL_Thread *    decode_tid;
    SDL_Thread *    video_tid;

    char filename[1024];
    int quit;
    long    maxFramesToDecode;
    int     currentFrameIndex;
};

SDL_Window * screen;
SDL_mutex * screen_mutex;
VideoState * global_video_state;
SDL_TimerID timer_id;

PacketQueue  audioq;
int quit = 0;

int video_thread(void * arg);
int stream_component_open(VideoState * videoState);
int decode_thread(void * data);

void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    Packet *pkt1;
    pkt1 = (Packet*)av_malloc(sizeof(Packet));
    if (!pkt1)
        return -1;
    //链表的方式；指向当前数据，并且下一个节点为NULL
    pkt1->av_packet = *pkt;
    pkt1->next = NULL;
    //在将数据加入到链队列之前首先上锁
    SDL_LockMutex(q->mutex);
    //首先判断当前的链队列是否为空，如果为空，则当前的节点作为第一个节点；否则接在链队列最后一个节点后面
    if (!q->last_pkt)
        q->first_pkt = pkt1;//指向队列头
    else
        q->last_pkt->next = pkt1;
    //将当前链队列指针向后移动一个位置，指向队列尾
    q->last_pkt = pkt1;
    //当前队列存储的packet数据数量
    q->nb_packets++;
    //当前的链队列大小（pkt.size当前存储的音频数据或者视频帧数据大小）
    q->size += pkt1->av_packet.size;
    //通知一个信号
    SDL_CondSignal(q->cond);
    //解锁
    SDL_UnlockMutex(q->mutex);
    return 0;
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
    Packet *pkt1;
    int ret;
    //在从队列中取出packet数据之前，首先上锁
    SDL_LockMutex(q->mutex);
    while(true) {
        if(quit) {
            ret = -1;
            break;
        }
        //首先从链队列当前指向的头部开始得到第一个packet数据
        pkt1 = q->first_pkt;
        //判断当前指向的头部数据是否为NULL，如果不为NULL，就从当前开始
        if (pkt1) {
            //指向头部的指针向后移动一个位置
            q->first_pkt = pkt1->next;
            //如果为NULL，表示到达最后一个位置
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;//packet数据减少一个
            q->size -= pkt1->av_packet.size;
            *pkt = pkt1->av_packet;//回收资源
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            //如果不满足条件，就等待条件信号的通知
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}

int audio_decode_frame(VideoState * videoState, uint8_t *audio_buf) {

    AVPacket * pkt = av_packet_alloc();
    static uint8_t * audio_pkt_data = nullptr;
    static int audio_pkt_size = 0;
    AVFrame * audioFrame = av_frame_alloc();
    int samples = 1024;
    int got_frame = -1;

    int len1, data_size = 0;

    while(true) {
        if(videoState -> quit)break;
        while(audio_pkt_size > 0) {
            got_frame = avcodec_send_packet(videoState -> audio_ctx,pkt);
            got_frame = avcodec_receive_frame(videoState -> audio_ctx,audioFrame);
            //计算读取的音频数据长度
            len1 = pkt -> size;
            if(len1 < 0) {
                audio_pkt_size = 0;
                break;
            }
            audio_pkt_data += len1;
            audio_pkt_size -= len1;
            data_size = 0;

            if(got_frame == 0) {
                enum AVSampleFormat sample_fmt = AV_SAMPLE_FMT_S16;
                data_size = av_samples_get_buffer_size(audioFrame -> linesize,
                                                       videoState -> audio_ctx -> ch_layout.nb_channels,
                                                       samples,sample_fmt,0);

                SDL_Log("sample buffer size is %d",data_size);
                uint8_t * audio_buffer = (uint8_t * )av_malloc(data_size);
                int ret = av_samples_fill_arrays(audioFrame -> data,audioFrame -> linesize,audio_buffer,
                                                 videoState -> audio_ctx -> ch_layout.nb_channels,samples,
                                                 sample_fmt,0);

                if(ret < 0){
                    SDL_Log("av samples fill arrays is failed");
                    avcodec_free_context(&videoState -> audio_ctx);
                    SDL_Quit();
                    return 0;
                }

                struct SwrContext * convert_context = swr_alloc();
                AVChannelLayout channelLayout;
                av_channel_layout_default(&channelLayout,2);

                ret = swr_alloc_set_opts2(&convert_context,&channelLayout,AV_SAMPLE_FMT_S16,44100,
                                          &channelLayout,videoState -> audio_ctx -> sample_fmt,
                                          videoState -> audio_ctx -> sample_rate,0,nullptr);
                if(ret < 0){
                    SDL_Log("set/reset parameters is failed");
                    avcodec_free_context(&videoState -> audio_ctx);
                    SDL_Quit();
                    return 0;
                }
                swr_init(convert_context);//注意不要忽略这一句：初始化 SWR 上下文
                int out_samples_max = av_rescale_rnd(swr_get_delay(convert_context, videoState -> audio_ctx -> sample_rate)
                                                         + samples,44100, videoState -> audio_ctx -> sample_rate, AV_ROUND_UP);

                SDL_Log("out samples max is %d",out_samples_max);
                swr_convert(convert_context,(uint8_t**)&audio_buffer,
                            out_samples_max,(const uint8_t **) audioFrame -> data,samples);

                memcpy(audio_buf, audio_buffer, data_size);
                //释放资源
                av_frame_free(&audioFrame);
                av_free(audio_buffer);
                swr_free(&convert_context);
            }
            if(data_size <= 0) {
                continue;
            }
            return data_size;
        }
        if(pkt -> data)av_packet_unref(pkt);
        if(quit) return -1;

        if(packet_queue_get(&videoState -> audioq, pkt, 1) < 0) {
            return -1;
        }
        audio_pkt_data = pkt -> data;
        audio_pkt_size = pkt -> size;
    }
}

void audio_callback(void *userdata, Uint8 *stream, int len) {

    VideoState * videoState = (VideoState *)userdata;
    int len1, audio_size = 0;

    while(len > 0) {
        if(videoState -> quit)return;

        if(videoState -> audio_buf_index >= videoState -> audio_buf_size) {
            //对读取的音频数据进行解码
            audio_size = audio_decode_frame(videoState, videoState -> audio_buf);

            if(audio_size < 0) {
                videoState -> audio_buf_size = 1024;
                SDL_memset(videoState -> audio_buf, 0, videoState -> audio_buf_size);
            } else {
                videoState -> audio_buf_size = audio_size;
            }
            videoState -> audio_buf_index = 0;
        }
        len1 = videoState -> audio_buf_size - videoState -> audio_buf_index;
        if(len1 > len)len1 = len;

        SDL_memcpy(stream, (uint8_t *)videoState -> audio_buf + videoState -> audio_buf_index, len1);
        len -= len1;
        stream += len1;
        videoState -> audio_buf_index += len1;
    }
}

static Uint32 sdl_refresh_timer_cb(Uint32 interval, void * opaque){
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    event.user.data1 = opaque;
    SDL_PushEvent(&event);
    //返回0表示停止定时器
    return 0;
}

//定时器触发的时间间隔，以毫秒为单位。例如，定时器每 delay 毫秒（1 秒）触发一次，
static void schedule_refresh(VideoState * videoState, int delay){
    timer_id = SDL_AddTimer(delay, sdl_refresh_timer_cb, videoState);
}

//如果存在错误的话就返回-1，表示线程异常结束
int failFun(VideoState * videoState){
    SDL_Event event;
    event.type = FF_QUIT_EVENT;
    event.user.data1 = videoState;
    SDL_PushEvent(&event);
    return -1;
}

//第一获取视频流和音频流索引和相关解码信息；第二将读取的视频帧和音频数据加入到指定的视频和音频队列中
int decode_thread(void * data){
    VideoState * videoState = (VideoState*)data;

    AVFormatContext *pFormatCtx = NULL;
    const char * fileName = videoState -> filename;
    if(avformat_open_input(&pFormatCtx,fileName, NULL, NULL) != 0){
        SDL_Log("open file is failed! and %s",SDL_GetError());
        SDL_Quit();
        return 0;
    }

    if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
        SDL_Log("not find stream information and %s",SDL_GetError());
        SDL_Quit();
        return 0;
    }
    //打印有关输入或输出格式的详细信息，如持续时间、比特率、流、容器、程序、元数据、侧数据、编解码器和时基。
    av_dump_format(pFormatCtx, 0, fileName, 0);

    AVCodecContext * aCodecCtx = nullptr;
    AVCodecContext * pCodecCtx = nullptr;
    // 找到视频流和音频流
    int videoStream = -1;
    int audioStream = -1;
    for(unsigned int i = 0; i < pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO && videoStream < 0) {
            videoStream=i;
            //视频帧部分
            const AVCodec * pCodec = nullptr;
            //找到一个与ID匹配的并且已经注册的解码器
            pCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
            //分配AVCodecContext并将其字段设置为默认值
            //以存储与找到的解码器相关的信息。这个上下文将用于后续的解码操作。
            pCodecCtx = avcodec_alloc_context3(pCodec);
            //根据提供的编解码器参数中的值填充编解码器上下文。
            //根据当前流的编码参数填充 AVCodecContext。这包括解析度、比特率、帧率等重要信息，使解码器知道如何正确解码该视频流。
            avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
            //根据解码器对上下文进行初始化;将所选解码器与上下文关联，以便可以开始解码过程。
            avcodec_open2(pCodecCtx, pCodec, nullptr);
        }
        if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO && audioStream < 0) {
            audioStream=i;
            //找到音频相关的解码信息
            const AVCodec * aCodec = nullptr;
            aCodec = avcodec_find_decoder(pFormatCtx -> streams[audioStream] -> codecpar -> codec_id);
            if(!aCodec) {
                SDL_Log("Unsupported codec! and %s\n",SDL_GetError());
                return failFun(videoState);
            }
            //将解码信息保存到aCodecCtx中
            aCodecCtx = avcodec_alloc_context3(aCodec);
            avcodec_parameters_to_context(aCodecCtx, pFormatCtx->streams[audioStream]->codecpar);
            //将所选解码器与上下文关联，以便可以开始解码过程。
            avcodec_open2(aCodecCtx, aCodec, NULL);
        }
    }

    if(videoStream == -1)
        return failFun(videoState);
    if(audioStream == -1)
        return failFun(videoState);
    SDL_Log("video and audio stream index is : [%d  %d]",videoStream,audioStream);

    videoState -> videoStream = videoStream;
    videoState -> audioStream = audioStream;
    videoState -> pFormatCtx = pFormatCtx;
    videoState -> audio_ctx = aCodecCtx;
    videoState -> video_ctx = pCodecCtx;
    //相关音频参数配置;视频和音频链队列初始化操作以及用于图像缩放的
    stream_component_open(videoState);

    AVPacket * packet = av_packet_alloc();

    while(true){
        if (videoState->quit){
            break;
        }
        //判断当前的队列大小和指定的最大视频帧和音频队列大小
        // if (videoState->audioq.size > MAX_AUDIOQ_SIZE || videoState->videoq.size > MAX_VIDEOQ_SIZE){
        //     SDL_Delay(10);
        //     continue;
        // }
        //读取视频帧和音频数据
        if (av_read_frame(videoState->pFormatCtx, packet) < 0){
            if (videoState->pFormatCtx->pb->error == 0){
                SDL_Delay(100);
                continue;
            }else{
                break;
            }
        }
        //将视频帧和音频数据加入链队列
        if (packet->stream_index == videoStream){
            packet_queue_put(&videoState->videoq, packet);
        }else if (packet->stream_index == audioStream){
            packet_queue_put(&videoState->audioq, packet);
        }else{
            av_packet_unref(packet);
        }
    }

    //由于有可能一个视频的帧数大于指定的视频队列大小，因此需要等待剩余部分执行完毕
    while (!videoState->quit){
        SDL_Delay(100);
    }

    //线程正常结束返回的状态
    av_free(pCodecCtx);
    av_free(aCodecCtx);
    return 0;
}
//相关音频参数配置;视频和音频链队列初始化操作以及用于图像缩放的
int stream_component_open(VideoState * videoState){

    AVFormatContext * pFormatCtx = videoState -> pFormatCtx;
    videoState -> audio_st = pFormatCtx -> streams[videoState -> audioStream];
    videoState -> audio_buf_size = 0;
    videoState -> audio_buf_index = 0;

    SDL_memset(&videoState -> audio_pkt,0,sizeof(videoState -> audio_pkt));
    packet_queue_init(&videoState -> audioq);

    //视频帧部分的相关参数以及变量
    videoState -> video_st = pFormatCtx -> streams[videoState -> videoStream];
    packet_queue_init(&videoState -> videoq);

    videoState -> video_tid = SDL_CreateThread(video_thread,"videoPlayer",videoState);

    //音频相关参数信息
    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = videoState -> audio_ctx -> sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = videoState -> audio_ctx -> ch_layout.nb_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = videoState;
    //打开音频设备
    if(SDL_OpenAudio(&wanted_spec, NULL) < 0) {
        SDL_Log("SDL_OpenAudio: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    SDL_PauseAudio(0);

    //分配和返回SwsContext，用于在sws_scale中执行对图像的缩放以及转换操作
    videoState -> sws_ctx = sws_getContext(videoState -> video_ctx -> width, videoState -> video_ctx -> height,
                                                videoState -> video_ctx->pix_fmt,videoState -> video_ctx -> width,
                                                videoState -> video_ctx -> height, AV_PIX_FMT_YUV420P,
                                                SWS_BILINEAR, nullptr, nullptr, nullptr);

    screen_mutex = SDL_CreateMutex();
    return 0;
}

void alloc_picture(void * userdata){
    VideoState * videoState = (VideoState *)userdata;

    VideoPicture * videoPicture;
    //取出一个视频帧（取出视频帧的原因是得到当前视频帧的信息）
    videoPicture = &videoState->pictq[videoState->pictq_windex];
    //将其占用的资源释放掉
    if (videoPicture->frame){
        av_frame_free(&videoPicture->frame);
        av_free(videoPicture->frame);
    }

    SDL_LockMutex(screen_mutex);

    //分配一个AVFrame并将其字段设置为默认值。
    videoPicture -> frame = av_frame_alloc();
    SDL_Log("width and height = (%d  %d)",videoState -> video_ctx -> width,videoState -> video_ctx -> height);
    int size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                        videoState -> video_ctx -> width,
                                        videoState -> video_ctx -> height, 1);
    uint8_t * video_buffer = (uint8_t *)av_malloc(size);
    //根据得到视频帧信息，分配缓冲区并在一次调用中填充dst_data和dst_linesize
    //根据指定的图像参数和提供的数组设置数据指针和行大小
    int ret = av_image_fill_arrays(videoPicture -> frame -> data, videoPicture -> frame -> linesize,
                                   video_buffer, AV_PIX_FMT_YUV420P,
                                   videoState -> video_ctx -> width, videoState -> video_ctx -> height, 1);
    if(ret < 0){
        SDL_Log("av image fill arrays is failed");
        avcodec_free_context(&videoState -> video_ctx);
        SDL_Quit();
        return;
    }

    SDL_UnlockMutex(screen_mutex);

    // 根据当前视频帧的信息，更新当前高宽
    videoPicture->width = videoState->video_ctx->width;
    videoPicture->height = videoState->video_ctx->height;
    videoPicture->allocated = 1;
}

int queue_picture(VideoState * videoState, AVFrame * pFrame){
    SDL_LockMutex(videoState->pictq_mutex);
    while (videoState->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && !videoState->quit){
        SDL_CondWait(videoState->pictq_cond, videoState->pictq_mutex);
    }
    SDL_UnlockMutex(videoState->pictq_mutex);

    if (videoState->quit){
        return -1;
    }

    //获得当前要缩放的视频帧
    VideoPicture * videoPicture;
    videoPicture = &videoState->pictq[videoState->pictq_windex];

    if (!videoPicture->frame ||
        videoPicture->width != videoState->video_ctx->width ||
        videoPicture->height != videoState->video_ctx->height){

        videoPicture->allocated = 0;
        alloc_picture(videoState);
        if(videoState->quit){
            return -1;
        }
    }
    //判断是否对frame数据设置了指针和大小，便于保存转换之后的图像信息
    if (videoPicture->frame){
        // set VideoPicture AVFrame info using the last decoded frame
        videoPicture->frame->pict_type = pFrame->pict_type;
        videoPicture->frame->pts = pFrame->pts;
        videoPicture->frame->pkt_dts = pFrame->pkt_dts;
        videoPicture->frame->width = pFrame->width;
        videoPicture->frame->height = pFrame->height;

        // 对当前的视频帧pFrame按照之前指定的高宽进行缩放，缩放的结果保存到videoPicture -> frame中
        int ret = sws_scale(
            videoState->sws_ctx,
            pFrame->data,pFrame->linesize,
            0,
            videoState->video_ctx->height,
            videoPicture->frame->data,
            videoPicture->frame->linesize
        );
        if(ret < 0)return -1;

        //更新当前视频帧的存放索引
        ++videoState->pictq_windex;
        if(videoState->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE){
            videoState->pictq_windex = 0;
        }

        SDL_LockMutex(videoState->pictq_mutex);
        //增加保存视频帧队列的大小
        videoState->pictq_size++;
        SDL_UnlockMutex(videoState->pictq_mutex);
    }
    return 0;
}

int video_thread(void * arg){
    VideoState * videoState = (VideoState *)arg;

    AVPacket * packet = av_packet_alloc();
    if (packet == NULL){
        printf("Could not alloc packet.\n");
        return -1;
    }
    int frameFinished;

    static AVFrame * pFrame = av_frame_alloc();
    if (!pFrame){
        printf("Could not allocate AVFrame.\n");
        return -1;
    }

    while(true){
        //如果当前视频链队列中没有视频帧就退出循环
        if (packet_queue_get(&videoState->videoq, packet, 1) < 0){
            break;
        }
        //对其视频帧数据进行解码操作
        int ret = avcodec_send_packet(videoState->video_ctx, packet);
        if (ret < 0){
            SDL_Log("Error sending packet for decoding.\n");
            return -1;
        }

        while (ret >= 0){
            //获得解码之后的视频帧数据
            ret = avcodec_receive_frame(videoState->video_ctx, pFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;
            }else if (ret < 0){
                SDL_Log("Error while decoding.\n");
                return -1;
            }else{
                frameFinished = 1;
            }
            //将解码之后的视频帧数据进行缩放等操作并跳出循环
            if (frameFinished){
                if(queue_picture(videoState, pFrame) < 0){
                    break;
                }
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&pFrame);
    av_free(pFrame);
    return 0;
}
//创建窗口和渲染器并显示视频帧
void video_display(VideoState * videoState){
    if (!screen){
        screen = SDL_CreateWindow(
            "FFmpeg SDL Video Player",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            videoState->video_ctx->width / 2,
            videoState->video_ctx->height / 2,
            SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI
            );
    }
    if (!screen){
        SDL_Log("SDL: could not create window - exiting.\n");
        return;
    }

    if (!videoState->renderer){
        videoState->renderer = SDL_CreateRenderer(screen, -1,
                                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    }
    if (!videoState->texture){
        videoState->texture = SDL_CreateTexture(
            videoState->renderer,
            SDL_PIXELFORMAT_YV12,
            SDL_TEXTUREACCESS_STREAMING,
            videoState->video_ctx->width,
            videoState->video_ctx->height
        );
    }

    VideoPicture * videoPicture;
    float aspect_ratio;
    int w, h, x, y;

    //从队列中获取要渲染的视频帧
    videoPicture = &videoState->pictq[videoState->pictq_rindex];
    //如果当前视频帧存在
    if (videoPicture->frame){
        //AVRtional: 通常用于处理与时间、帧率、比特率等相关的数值，尤其是在音视频处理领域。
        if (videoState->video_ctx->sample_aspect_ratio.num == 0){
            aspect_ratio = 0;
        }else{
            //其中sample_aspect_ratio这是像素的宽度除以像素的高度
            //av_q2d 函数用于将 AVRational 类型的有理数转换为双精度浮点数（double）。
            //这个函数的主要作用是将表示为分数的值（如帧率、比特率等）转换为更易于处理的浮点数格式
            aspect_ratio = av_q2d(videoState->video_ctx->sample_aspect_ratio) * \
                           videoState->video_ctx->width / videoState->video_ctx->height; //得到视频帧的实际高宽比
        }
        //计算视频帧的高宽比
        if (aspect_ratio <= 0.0){
            aspect_ratio = (float)videoState->video_ctx->width /
                           (float)videoState->video_ctx->height;
        }

        int screen_width;
        int screen_height;
        //获得当前创建的窗口大小
        SDL_GetWindowSize(screen, &screen_width, &screen_height);
        h = screen_height;
        w = ((int) rint(h * aspect_ratio)) & - 3; // 四舍五入到最近的整数
        if (w > screen_width){
            w = screen_width;
            h = ((int) rint(w / aspect_ratio)) & -3;
        }

        x = (screen_width - w);
        y = (screen_height - h);

        SDL_Rect rect;
        rect.x = x;
        rect.y = y;
        rect.w = 2*w;
        rect.h = 2*h;

        SDL_LockMutex(screen_mutex);
        SDL_UpdateYUVTexture(
            videoState->texture,
            &rect,
            videoPicture->frame->data[0],
            videoPicture->frame->linesize[0],
            videoPicture->frame->data[1],
            videoPicture->frame->linesize[1],
            videoPicture->frame->data[2],
            videoPicture->frame->linesize[2]
            );

        SDL_RenderClear(videoState->renderer);
        SDL_RenderCopy(videoState->renderer, videoState->texture, NULL, NULL);
        SDL_RenderPresent(videoState->renderer);
        SDL_UnlockMutex(screen_mutex);
    }
}

void video_refresh_timer(void * userdata){
    VideoState * videoState = (VideoState *)userdata;

    if (videoState->video_st){
        //判断当前队列中是否有视频帧
        if (videoState->pictq_size == 0){
            schedule_refresh(videoState, 39);
        }else{
            //获得当前要渲染的视频帧
            VideoPicture * videoPicture = &videoState->pictq[videoState->pictq_rindex];
            //39ms触发一次定时器，同时就要push一次FF_REFRESH_EVENT，同时调用video_refresh_timer
            schedule_refresh(videoState, 39);
            //显示视频帧
            video_display(videoState);
            if(++videoState->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE){
                videoState->pictq_rindex = 0;
            }
            SDL_LockMutex(videoState->pictq_mutex);
            videoState->pictq_size--;
            SDL_CondSignal(videoState->pictq_cond);
            SDL_UnlockMutex(videoState->pictq_mutex);
        }
    }else{
        schedule_refresh(videoState, 39);
    }
}



int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    avdevice_register_all();
    //分配一组用户定义的事件，并返回该事件集的起始事件号。
    int bEvent = SDL_RegisterEvents(2);
    if(bEvent == -1){
        SDL_Log("allocate event is failed and %s!",SDL_GetError());
        SDL_Quit();
        return 0;
    }
    // 初始化 SDL
    int ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    if(ret < 0){
        SDL_Log("initialize video is failed and %s",SDL_GetError());
        SDL_Quit();
        return 0;
    }

    SDL_version v;
    SDL_VERSION(&v);
    qDebug()<<"SDL2 version: "<<v.major<<" "<<v.minor<<" "<<v.patch;
    qDebug()<<"ffmpeg version: "<<av_version_info();

    const char * fileName = "D:\\SoftwareFamily\\QT\\projects\\QtFFmpeg_Application\\QtSDLFFmpeg_videoPlayer_v1\\resources\\GuoYongZhe.mp4";
    // const char * fileName = "D:\\SoftwareFamily\\QT\\projects\\QtVideoPlayer_v2\\resources\\videos\\GuoYongZhe.mp4";

    VideoState * videoState = NULL;
    videoState = (VideoState*)av_mallocz(sizeof(VideoState));

    //初始化videoState -> filename的文件路径
    SDL_memcpy(videoState -> filename,fileName,sizeof(videoState -> filename));
    SDL_Log("file path is %s",videoState -> filename);

    videoState -> pictq_mutex = SDL_CreateMutex();
    videoState -> pictq_cond = SDL_CreateCond();

    schedule_refresh(videoState,39);
    videoState -> decode_tid = SDL_CreateThread(decode_thread,"decodeThread",videoState);
    if(!videoState -> decode_tid){
        SDL_Log("create thread is failed and %s",SDL_GetError());
        av_free(videoState);
        SDL_DestroyCond(videoState -> pictq_cond);
        SDL_DestroyMutex(videoState -> pictq_mutex);
        SDL_Quit();
        return 0;
    }

    SDL_Event event;
    while(true){
        SDL_WaitEvent(&event);
        switch(event.type){
            case FF_QUIT_EVENT:
                videoState -> quit = 1;
                SDL_Quit();
                break;
            case SDL_QUIT:
                videoState->quit = 1;
                SDL_Quit();
                break;
            case SDL_WINDOWEVENT:
                if(event.window.event == SDL_WINDOWEVENT_CLOSE){
                    if(event.window.windowID == SDL_GetWindowID(screen)){
                        SDL_DestroyWindow(screen);
                        videoState -> quit = 1;
                    }
                }
                SDL_Quit();
                break;
            case FF_REFRESH_EVENT:{
                video_refresh_timer(event.user.data1);
                break;
            default:
                break;
            }
        }
        if (videoState->quit){
            break;
        }
    }
    int status = 0;
    SDL_threadID decode_id = SDL_GetThreadID(videoState -> decode_tid);
    SDL_Log("decode function thread id is %lu",decode_id);
    SDL_WaitThread(videoState -> decode_tid,&status);
    if(status == -1){
        SDL_Log("线程函数 decode thread 异常退出!");
    }else{
        SDL_Log("线程函数 decode thread 正常退出!");
    }
    SDL_threadID video_id = SDL_GetThreadID(videoState -> video_tid);
    SDL_Log("video function thread id is %lu",video_id);
    SDL_WaitThread(videoState -> video_tid,&status);
    if(status == -1){
        SDL_Log("线程函数 video thread 异常退出!");
    }else{
        SDL_Log("线程函数 video thread 正常退出!");
    }
    SDL_CloseAudio();
    av_free(videoState);
    SDL_RemoveTimer(timer_id);
    SDL_Quit();
    return 0;
}
