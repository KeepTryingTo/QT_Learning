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

PacketQueue  audioq;
int quit = 0;

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

int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf) {

    AVPacket * pkt = av_packet_alloc();
    static uint8_t * audio_pkt_data = nullptr;
    static int audio_pkt_size = 0;
    AVFrame * audioFrame = av_frame_alloc();
    int samples = 1024;
    int got_frame = -1;

    int len1, data_size = 0;

    while(true) {
        while(audio_pkt_size > 0) {
            got_frame = avcodec_send_packet(aCodecCtx,pkt);
            got_frame = avcodec_receive_frame(aCodecCtx,audioFrame);
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
                                                      aCodecCtx -> ch_layout.nb_channels,
                                                      samples,sample_fmt,0);

                SDL_Log("sample buffer size is %d",data_size);
                uint8_t * audio_buffer = (uint8_t * )av_malloc(data_size);
                int ret = av_samples_fill_arrays(audioFrame -> data,audioFrame -> linesize,audio_buffer,
                                                 aCodecCtx -> ch_layout.nb_channels,samples,
                                                 sample_fmt,0);

                if(ret < 0){
                    SDL_Log("av samples fill arrays is failed");
                    avcodec_free_context(&aCodecCtx);
                    SDL_Quit();
                    return 0;
                }

                struct SwrContext * convert_context = swr_alloc();
                AVChannelLayout channelLayout;
                av_channel_layout_default(&channelLayout,2);

                ret = swr_alloc_set_opts2(&convert_context,&channelLayout,AV_SAMPLE_FMT_S16,44100,
                                          &channelLayout,aCodecCtx -> sample_fmt,
                                          aCodecCtx -> sample_rate,0,nullptr);
                if(ret < 0){
                    SDL_Log("set/reset parameters is failed");
                    avcodec_free_context(&aCodecCtx);
                    SDL_Quit();
                    return 0;
                }
                swr_init(convert_context);//注意不要忽略这一句：初始化 SWR 上下文
                int out_samples_max = av_rescale_rnd(swr_get_delay(convert_context, aCodecCtx -> sample_rate)
                                                    + samples,44100, aCodecCtx -> sample_rate, AV_ROUND_UP);

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

        if(packet_queue_get(&audioq, pkt, 1) < 0) {
            return -1;
        }
        audio_pkt_data = pkt -> data;
        audio_pkt_size = pkt -> size;
    }
}

void audio_callback(void *userdata, Uint8 *stream, int len) {

    AVCodecContext *aCodecCtx = (AVCodecContext *)userdata;
    int len1, audio_size;

    SDL_Log("audio callback ...");

    static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;

    while(len > 0) {
        if(audio_buf_index >= audio_buf_size) {
            //对读取的音频数据进行解码
            audio_size = audio_decode_frame(aCodecCtx, audio_buf);

            if(audio_size < 0) {
                audio_buf_size = 1024;
                memset(audio_buf, 0, audio_buf_size);
            } else {
                audio_buf_size = audio_size;
            }
            audio_buf_index = 0;
        }
        len1 = audio_buf_size - audio_buf_index;
        if(len1 > len)len1 = len;
        memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    avdevice_register_all();
    // 初始化 SDL
    int ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    if(ret < 0){
        SDL_Log("initialize video is failed and %s",SDL_GetError());
        SDL_Quit();
        return 0;
    }

    // SDL_GL_SetSwapInterval(1);
    //初始化链队列
    packet_queue_init(&audioq);

    // 创建 SDL 窗口
    SDL_Window *window = nullptr;
    SDL_Renderer * renderer = nullptr;

    // 获取当前视频显示
    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        SDL_Log("SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_version v;
    SDL_VERSION(&v);
    qDebug()<<"SDL2 version: "<<v.major<<" "<<v.minor<<" "<<v.patch;
    qDebug()<<"ffmpeg version: "<<av_version_info();

    const char * fileName = "D:\\SoftwareFamily\\QT\\projects\\QtFFmpeg_Application\\QtSDLFFmpeg_videoPlayer_v1\\resources\\GuoYongZhe.mp4";
    // const char * fileName = "D:\\SoftwareFamily\\QT\\projects\\QtVideoPlayer_v2\\resources\\videos\\GuoYongZhe.mp4";
    AVFormatContext *pFormatCtx = NULL;

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
                fprintf(stderr, "Unsupported codec!\n");
                return -1;
            }
            //将解码信息保存到aCodecCtx中
            aCodecCtx = avcodec_alloc_context3(aCodec);
            avcodec_parameters_to_context(aCodecCtx, pFormatCtx->streams[audioStream]->codecpar);
            //将所选解码器与上下文关联，以便可以开始解码过程。
            avcodec_open2(aCodecCtx, aCodec, NULL);
        }
    }
    if(videoStream==-1)
        return -1;
    if(audioStream==-1)
        return -1;
    SDL_Log("video and audio stream index is : [%d  %d]",videoStream,audioStream);

    //音频相关参数信息
    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = aCodecCtx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = aCodecCtx -> ch_layout.nb_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = aCodecCtx;
    //打开音频设备
    if(SDL_OpenAudio(&wanted_spec, NULL) < 0) {
        SDL_Log("SDL_OpenAudio: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    SDL_PauseAudio(0);

    //分配一个AVFrame并将其字段设置为默认值。
    AVFrame * pFrame = av_frame_alloc();
    AVFrame * pFrameRGB = av_frame_alloc();
    SDL_Log("width and height = (%d  %d)",pCodecCtx -> width,pCodecCtx -> height);
    int size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                        pCodecCtx -> width,
                                        pCodecCtx -> height, 1);
    uint8_t * video_buffer = (uint8_t *)av_malloc(size);
    //分配缓冲区并在一次调用中填充dst_data和dst_linesize
    ret = av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize,
                               video_buffer, AV_PIX_FMT_YUV420P,
                               pCodecCtx -> width, pCodecCtx -> height, 1);
    if(ret < 0){
        SDL_Log("av image fill arrays is failed");
        avcodec_free_context(&pCodecCtx);
        SDL_Quit();
        return 0;
    }
    //分配和返回SwsContext，用于在sws_scale中执行对图像的缩放以及转换操作
    struct SwsContext * sws_ctx = sws_getContext(pCodecCtx -> width, pCodecCtx -> height,
                                                 pCodecCtx->pix_fmt,pCodecCtx -> width,
                                                 pCodecCtx -> height, AV_PIX_FMT_YUV420P,
                                                 SWS_BILINEAR, nullptr, nullptr, nullptr);

    // 计算窗口应该放置的x和y坐标，以使其居中 ；这里之所以分别创建窗口和渲染器，主要是因为方便关闭窗口
    int windowWidth = pCodecCtx -> width;
    int windowHeight = pCodecCtx -> height;
    int screenWidth = dm.w;
    int screenHeight = dm.h;
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    window = SDL_CreateWindow("video player",
                              x,y,
                              pCodecCtx -> width,
                              pCodecCtx -> height,
                              0);
    renderer = SDL_CreateRenderer(window,-1,0);
    if(renderer == nullptr){
        SDL_Log("initialize window and renderer is failed and %s",SDL_GetError());
        SDL_Quit();
        return 0;
    }

    //创建用于渲染上下文的纹理
    SDL_Texture * texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
                                              pCodecCtx -> width, pCodecCtx -> height);
    AVPacket * packet = av_packet_alloc();
    SDL_Event e;

    while(av_read_frame(pFormatCtx, packet) >= 0) {
        if(packet -> stream_index == videoStream) {
            //提供了录制、转换以及流化音频和视频的能力。在编码过程中，avcodec_send_packet 函数将原始数据包（如从文件、
            //网络或摄像头捕获的帧）发送到编码器以供处理的角色。
            avcodec_send_packet(pCodecCtx, packet);
            //获取编码后的数据
            while (avcodec_receive_frame(pCodecCtx, pFrame) >= 0) {
                //在srcSlice中缩放图像切片，并将缩放后的切片放入dst中的图像中。切片是图像中连续行的序列。
                //将pFrame图像切片缩放之后存储到pFrameRGB中;返回图像切片的高度
                sws_scale(sws_ctx, pFrame->data, pFrame->linesize,0,
                          pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

                SDL_UpdateTexture(texture, nullptr, pFrameRGB->data[0], pFrameRGB->linesize[0]);
                // // the area of the texture to be updated
                // SDL_Rect rect;
                // rect.x = 0;
                // rect.y = 0;
                // rect.w = pCodecCtx->width;
                // rect.h = pCodecCtx->height;

                // // update the texture with the new pixel data
                // SDL_UpdateYUVTexture(
                //     texture,
                //     &rect,
                //     pFrameRGB->data[0],
                //     pFrameRGB->linesize[0],

                //     pFrameRGB->data[1],
                //     pFrameRGB->linesize[1],

                //     pFrameRGB->data[2],
                //     pFrameRGB->linesize[2]
                // );

                SDL_RenderClear(renderer);//清除渲染器缓冲区的内容
                SDL_RenderCopy(renderer, texture, nullptr, nullptr);
                SDL_RenderPresent(renderer);
            }
        }
        else if(packet -> stream_index == audioStream) {
            packet_queue_put(&audioq, packet);
        }
        else{
            av_packet_unref(packet);
        }
        SDL_PollEvent(&e);//用于从事件队列中获取待处理的事件
        if(e.type == SDL_QUIT){
            SDL_Quit();
            quit = 1;
        }
        if(quit)break;
        SDL_Delay(10);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
    avcodec_free_context(&pCodecCtx);
    avcodec_free_context(&aCodecCtx);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameRGB);
    av_packet_free(&packet);
    SDL_CloseAudio();
    avformat_close_input(&pFormatCtx);
    SDL_Quit();

    return 0;
}
