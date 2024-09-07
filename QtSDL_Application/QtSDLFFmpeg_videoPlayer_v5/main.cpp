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
#include <libavutil/time.h>
#include <libavutil/frame.h>
}

// #include <queue>

#undef main

#include <QQueue>
#define _DEBUG_ 0
#define FF_REFRESH_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 1)
#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)
#define VIDEO_PICTURE_QUEUE_SIZE 1
#define SDL_AUDIO_BUFFER_SIZE 1024


#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 0.1
#define SAMPLE_CORRECTION_PERCENT_MAX 10
#define AUDIO_DIFF_AVG_NB 20
#define DEFAULT_AV_SYNC_TYPE AV_SYNC_AUDIO_MASTER


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
    double      pts;
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

    double              frame_timer;
    double              frame_last_pts;
    double              frame_last_delay;
    double              video_clock;
    double              audio_clock;

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

    double              video_current_pts;
    int64_t             video_current_pts_time;
    double              audio_diff_cum;
    double              audio_diff_avg_coef;
    double              audio_diff_threshold;
    int                 audio_diff_avg_count;

    int                 av_sync_type;
    double              external_clock;
    int64_t             external_clock_time;

    SDL_Thread *        decode_tid;
    SDL_Thread *        video_tid;

    char                filename[1024];
    int                 quit;
    long                maxFramesToDecode;
    int                 currentFrameIndex;

    int                 seek_req;
    int                 seek_flags;
    int64_t             seek_pos;
};

//分别对于音频，视频以及计算机本身的异步时钟
enum{
    AV_SYNC_AUDIO_MASTER,
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_MASTER,
};

SDL_Window * screen;
SDL_mutex * screen_mutex;
VideoState * global_video_state;
SDL_TimerID timer_id;

PacketQueue  audioq;
int quit = 0;
AVPacket * flush_pkt;

int video_thread(void * arg);
int stream_component_open(VideoState * videoState);
int decode_thread(void * data);
int synchronize_audio(VideoState * videoState, short * samples, int samples_size);
double get_external_clock(VideoState * videoState);
double get_master_clock(VideoState * videoState);
double get_audio_clock(VideoState * videoState);
double get_video_clock(VideoState * videoState);
static void packet_queue_flush(PacketQueue * queue);
void stream_seek(VideoState * videoState, int64_t pos, int rel);

void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    Packet *pkt1;
    pkt1 = (Packet*)av_malloc(sizeof(Packet));
    if (!pkt1 && pkt != flush_pkt)
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
        if (global_video_state->quit){
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

int audio_decode_frame(VideoState * videoState, uint8_t *audio_buf,double * pts_ptr) {

    AVPacket * pkt = av_packet_alloc();
    static uint8_t * audio_pkt_data = nullptr;
    static int audio_pkt_size = 0;
    AVFrame * audioFrame = av_frame_alloc();
    int samples = SDL_AUDIO_BUFFER_SIZE;
    int got_frame = -1;

    int len1, data_size = 0;
    double pts = 0;
    int n = 0;

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
                //根据给定给定音频的参数得到缓冲区大小
                data_size = av_samples_get_buffer_size(audioFrame -> linesize,
                                                       videoState -> audio_ctx -> ch_layout.nb_channels,
                                                       samples,sample_fmt,0);

                // SDL_Log("sample buffer size is %d",data_size);
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

                // SDL_Log("out samples max is %d",out_samples_max);
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

            pts = videoState -> audio_clock;
            *pts_ptr = pts;
            n = 2 * videoState -> audio_ctx -> ch_layout.nb_channels;
            //当前时钟 + 计算的当前时钟比率 = 播放data_size大小音频数据之后的时钟
            videoState -> audio_clock += (double)data_size / (double)(n * videoState -> audio_ctx -> sample_rate);
            return data_size;
        }
        if(pkt -> data)av_packet_unref(pkt);
        if(quit) return -1;

        if(packet_queue_get(&videoState -> audioq, pkt, 1) < 0) {
            return -1;
        }
        if (pkt->data == flush_pkt -> data){
            //数用于清空解码器的内部缓冲区。这在处理音视频流时非常重要，
            //尤其是在需要重新开始解码或在流的不同部分之间切换时。
            avcodec_flush_buffers(videoState->audio_ctx);
            continue;
        }
        audio_pkt_data = pkt -> data;
        audio_pkt_size = pkt -> size;

        if(pkt -> pts != AV_NOPTS_VALUE){
            videoState -> audio_clock = av_q2d(videoState -> audio_st -> time_base) * pkt -> pts;
        }
    }
    return -1;
}

void audio_callback(void *userdata, Uint8 *stream, int len) {
    VideoState * videoState = (VideoState *)userdata;
    int len1, audio_size = 0;
    double pts = 0;

    SDL_memset(stream, 0, len);

    while(len > 0) {
        if(global_video_state -> quit)return;

        if(videoState -> audio_buf_index >= videoState -> audio_buf_size) {
            //对读取的音频数据进行解码
            audio_size = audio_decode_frame(videoState, videoState -> audio_buf,&pts);

            if(audio_size < 0) {
                videoState -> audio_buf_size = SDL_AUDIO_BUFFER_SIZE;
                SDL_memset(videoState -> audio_buf, 0, videoState -> audio_buf_size);
            } else {
                audio_size = synchronize_audio(videoState,(short*)videoState -> audio_buf,audio_size);
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
//返回告诉要向流发送多少个字节，将采样集大小调整为wanted_size
int synchronize_audio(VideoState * videoState, short * samples, int samples_size){
    int n;
    double ref_clock;
    //这里乘以2，是因为音频格式为16位的
    n = 2 * videoState->audio_ctx->ch_layout.nb_channels;

    //判断当前的同步类型格式
    if (videoState->av_sync_type != AV_SYNC_AUDIO_MASTER){
        double diff, avg_diff;
        int wanted_size, min_size, max_size;

        //获得当前同步音频的时钟
        ref_clock = get_master_clock(videoState);
        //时间偏移的时钟差异
        diff = get_audio_clock(videoState) - ref_clock;

        if (diff < AV_NOSYNC_THRESHOLD){
            //累积音频播放造成的不同步差异
            videoState->audio_diff_cum = diff + videoState->audio_diff_avg_coef * videoState->audio_diff_cum;
            //比较当前记录的不同步音频采样集和的指定的N个采样集之间的关系，如果小于指定的N个采样集就继续++
            if (videoState->audio_diff_avg_count < AUDIO_DIFF_AVG_NB){
                videoState->audio_diff_avg_count++;
            }else{
                //求解N个不同步音频采样集之间的平均值
                avg_diff = videoState->audio_diff_cum * (1.0 - videoState->audio_diff_avg_coef);

                //如果不同步音频采样集的平均值大于指定的阈值就需要进一步处理
                if (fabs(avg_diff) >= videoState->audio_diff_threshold){
                    //当前的采样集大小 + diff不同期间采样集大小
                    wanted_size = samples_size + ((int)(diff * videoState->audio_ctx->sample_rate) * n);
                    //对校正之后的采样集大小进行限制，如果过多地更改缓冲区，对用户来说会太刺耳
                    min_size = samples_size * ((100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100);
                    max_size = samples_size * ((100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100);

                    if(wanted_size < min_size){
                        wanted_size = min_size;
                    }else if (wanted_size > max_size)
                    {
                        wanted_size = max_size;
                    }
                    //如果计算校正的采样集大小小于指定的缓冲区大小
                    if(wanted_size < samples_size){
                        samples_size = wanted_size;
                    }else if(wanted_size > samples_size){
                        SDL_Log(".........");
                        uint8_t *samples_end, *q;
                        int nb = (samples_size - wanted_size);
                        //根据采样集数据samples + samples_size - n = 数据samples末尾
                        samples_end = (uint8_t *)samples + samples_size - n;
                        q = samples_end + n;

                        SDL_Log("samples size is %d  nb = %d",samples_size,nb);

                        while(nb > 0){
                            memcpy(q, samples_end, n);
                            q += n;
                            nb -= n;
                        }

                        samples_size = wanted_size;
                    }
                }
            }
        }else{
            /* difference is TOO big; reset diff stuff */
            videoState->audio_diff_avg_count = 0;
            videoState->audio_diff_cum = 0;
        }
    }
    return samples_size;
}

double get_video_clock(VideoState * videoState){
    //当前时间和前一帧视频显示的时间间隔 => 单位从微秒转换为秒
    double delta = (av_gettime() - videoState->video_current_pts_time) / 1000000.0;

    return videoState->video_current_pts + delta;
}
double get_external_clock(VideoState * videoState){
    videoState->external_clock_time = av_gettime();
    //获得当前时间 => 单位从微秒转换为秒
    videoState->external_clock = videoState->external_clock_time / 1000000.0;
    return videoState->external_clock;
}

double get_master_clock(VideoState * videoState){
    //判断当前的是同步视频控制还是同步音频控制，或者是当前时间的同步控制
    if (videoState->av_sync_type == AV_SYNC_VIDEO_MASTER){
        return get_video_clock(videoState);
    }else if (videoState->av_sync_type == AV_SYNC_AUDIO_MASTER){
        return get_audio_clock(videoState);
    }else if (videoState->av_sync_type == AV_SYNC_EXTERNAL_MASTER){
        return get_external_clock(videoState);
    }else{
        fprintf(stderr, "Error: Undefined A/V sync type.");
        return -1;
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

static void packet_queue_flush(PacketQueue * queue){
    Packet *pkt, *pkt1;

    SDL_LockMutex(queue->mutex);
    //首先从链队列的头部开始遍历，释放每一个节点资源
    for (pkt = queue->first_pkt; pkt != NULL; pkt = pkt1){
        pkt1 = pkt->next;
        av_packet_unref(&pkt->av_packet);
        av_freep(&pkt);
    }
    //链队列资源释放完毕之后就初始化对应的指针
    queue->last_pkt = NULL;
    queue->first_pkt = NULL;
    queue->nb_packets = 0;
    queue->size = 0;

    SDL_UnlockMutex(queue->mutex);
}

void stream_seek(VideoState * videoState, int64_t pos, int rel){
    //判断当前指向的位置是否合法
    if (!videoState->seek_req){
        videoState->seek_pos = pos;
        //判断当前移动的位置是否越过边界
        //AVSEEK_FLAG_BACKWARD 是一个用于控制媒体流寻址行为的标志，
        //通常与 av_seek_frame 函数一起使用。它的主要作用是指示在进行时间戳寻址时，是否允许向后寻址。
        videoState->seek_flags = rel < 0 ? AVSEEK_FLAG_BACKWARD : 0;//后退还是前进标识
        videoState->seek_req = 1;
    }
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

    global_video_state = videoState;
    //相关音频参数配置;视频和音频链队列初始化操作以及用于图像缩放的
    stream_component_open(videoState);

    AVPacket * packet = av_packet_alloc();

    while(true){
        if (videoState->quit){
            break;
        }
        if(videoState -> seek_req){
            int video_stream_index = -1;
            int audio_stream_index = -1;
            //根据指定的位置POS，进行音频和视频帧的位置改变
            int64_t seek_target_video = videoState -> seek_pos;
            int64_t seek_target_audio = videoState -> seek_pos;

            if(videoState -> videoStream >= 0)video_stream_index = videoState -> videoStream;
            if(videoState -> audioStream >= 0)audio_stream_index = videoState -> audioStream;

            if(video_stream_index >= 0 && audio_stream_index >= 0){
                //av_rescale_q (a,b,c)是一个函数，它将时间戳从一个基数重新缩放到另一个基数。它基本上计算a*b/c
                seek_target_video = av_rescale_q(seek_target_video,AV_TIME_BASE_Q,pFormatCtx -> streams[video_stream_index] -> time_base);
                seek_target_audio = av_rescale_q(seek_target_audio,AV_TIME_BASE_Q,pFormatCtx -> streams[audio_stream_index] -> time_base);
            }
            //在时间戳中寻找关键的帧
            int ret = av_seek_frame(videoState -> pFormatCtx,video_stream_index,seek_target_video,videoState -> seek_flags);
            ret &= av_seek_frame(videoState -> pFormatCtx,audio_stream_index,seek_target_audio,videoState -> seek_flags);

            if(ret < 0){
                SDL_Log("error while seeking");
                return failFun(videoState);
            }
            if(videoState -> videoStream >= 0){
                packet_queue_flush(&videoState -> videoq);
                packet_queue_put(&videoState -> videoq,flush_pkt);
            }
            if(videoState -> audioStream >= 0){
                packet_queue_flush(&videoState -> audioq);
                packet_queue_put(&videoState -> audioq,flush_pkt);
            }
            videoState->seek_req = 0;
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
    avformat_close_input(&pFormatCtx);

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
    //记录打开视频时的时间 微秒转换为秒
    videoState->frame_timer = (double)av_gettime() / 1000000.0;
    videoState->frame_last_delay = 40e-3;//默认的时间延迟
    videoState->video_current_pts_time = av_gettime();//记录第一帧显示时间

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

    SDL_Log("channels = %d  freq = %d  format = %d",wanted_spec.channels,wanted_spec.freq,
            videoState -> audio_ctx -> sample_fmt);

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

int queue_picture(VideoState * videoState, AVFrame * pFrame,double pts){
    SDL_LockMutex(videoState->pictq_mutex);
    while (videoState->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && !videoState->quit){
        SDL_CondWait(videoState->pictq_cond, videoState->pictq_mutex);
        // SDL_Log("videoState->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE.......");
    }
    SDL_UnlockMutex(videoState->pictq_mutex);

    // SDL_Log("queue picture............");

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
        videoPicture -> pts = pts; //记录当前视频帧显示的时间戳
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

double synchronize_video(VideoState * videoState, AVFrame * src_frame, double pts){
    double frame_delay;
    //判断当前的显示时间戳是否为0
    if (pts != 0){
        //如果指定的PTS不为0，就设置时钟
        videoState->video_clock = pts;
    }else{
        pts = videoState->video_clock;
    }
    //将时间戳转换为double浮点数，time_base在视频播放中表示1秒 / 多少个时间单位（FPS），
    frame_delay = av_q2d(videoState->video_ctx->time_base);
    // 如果存在重复的帧，就根据时钟进行调整
    // repeat_pict  是一个整数值，表示当前帧的重复次数，变量用于指示当前帧是否需要重复播放
    // 其实这样计算可以得到实际的播放时间
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    //重复播放的时间延迟 + 当前视频帧播放的时钟 = 实际的播放时间
    videoState->video_clock += frame_delay;
    return pts;
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

    double pts = 0;

    while(true){
        //如果当前视频链队列中没有视频帧就退出循环
        if (packet_queue_get(&videoState->videoq, packet, 1) < 0){
            break;
        }
        //判断当前从队列中取出的数据是否为要刷新的数据
        if (packet->data == flush_pkt -> data){
            avcodec_flush_buffers(videoState->video_ctx);
            continue;
        }
        //对其视频帧数据进行解码操作
        int ret = avcodec_send_packet(videoState->video_ctx, packet);
        if (ret < 0){
            SDL_Log("Error sending packet for decoding.\n");
            return -1;
        }

        while (ret >= 0){
            pts = 0;
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
            //best_effort_timestamp 变量用于表示当前帧的最佳努力时间戳
            //可以用于代替显示时间戳PTS，表示当前帧实际的显示时间戳
            if(packet -> pts == AV_NOPTS_VALUE){
                pts = pFrame -> best_effort_timestamp;
            }else{
                pts = 0;
            }
            //video_st -> time_base时间戳和video_ctx->time_base时间戳是一个意思
            pts = pts * av_q2d(videoState -> video_st -> time_base);
            //将解码之后的视频帧数据进行缩放等操作并跳出循环
            if (frameFinished){
                //流中的帧的时间戳（PTS 和 DTS）通常是以 time_base 作为单位。通过将时间戳与 time_base 相乘，可以将其转换为秒
                //也就是相乘之后才是实际的显示时间秒
                pts = synchronize_video(videoState,pFrame,pts);
                if(queue_picture(videoState, pFrame,pts) < 0){
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
            videoState->video_ctx->width,
            videoState->video_ctx->height,
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
    }else{
        SDL_Event event;
        event.type = FF_QUIT_EVENT;
        event.user.data1 = videoState;

        // push the event
        SDL_PushEvent(&event);
    }
}

double get_audio_clock(VideoState * videoState){

    //当前音频播放时间钟
    double pts = videoState->audio_clock;
    //当前缓冲区大小 - 正在播放音频数据位置 = 当前可用大小
    int hw_buf_size = videoState->audio_buf_size - videoState->audio_buf_index;
    int bytes_per_sec = 0;
    //这里之所以要乘以2，主要是因为音频格式为16位，也就是2个字节大小
    int n = 2 * videoState->audio_ctx->ch_layout.nb_channels;
    //计算的是每秒钟音频数据的总字节数
    if (videoState->audio_st){
        bytes_per_sec = videoState->audio_ctx->sample_rate * n;
    }
    //pts - 当前剩余未播放部分的字节数和总字节数的比率 = 剩余未播放部分字节数之前的时间戳
    if (bytes_per_sec){
        pts -= (double) hw_buf_size / bytes_per_sec;
    }
    return pts;
}

void video_refresh_timer(void * userdata){
    VideoState * videoState = (VideoState *)userdata;

    double pts_delay;
    double audio_ref_clock;
    double sync_threshold;
    double real_delay;
    double audio_video_delay;

    if (videoState->video_st){
        //判断当前队列中是否有视频帧
        if (videoState->pictq_size == 0){
            schedule_refresh(videoState, 1);
        }else{
            VideoPicture * videoPicture = &videoState->pictq[videoState->pictq_rindex];

            if (_DEBUG_){
                printf("Current Frame PTS:\t\t%f\n", videoPicture->pts);
                printf("Last Frame PTS:\t\t\t%f\n", videoState->frame_last_pts);
            }
            //表示当前帧的时间戳- 前一帧播放的显示时间戳 = 两帧之间的播放延迟
            pts_delay = videoPicture->pts - videoState->frame_last_pts;
            if (_DEBUG_)SDL_Log("PTS Delay:\t\t\t\t%f\n", pts_delay);

            if (pts_delay <= 0 || pts_delay >= 1.0){
                pts_delay = videoState->frame_last_delay;
            }

            if (_DEBUG_)SDL_Log("Corrected PTS Delay:\t%f\n", pts_delay);
            //记录下一次的延迟时间
            videoState->frame_last_delay = pts_delay;
            videoState->frame_last_pts = videoPicture->pts;

            // 默认为AV_SYNC_AUDIO_MASTER
            if(videoState->av_sync_type != AV_SYNC_VIDEO_MASTER){
                //与主时钟保持同步的更新延迟:音频或视频
                audio_ref_clock = get_master_clock(videoState);

                if (_DEBUG_)SDL_Log("Ref Clock:\t\t%f\n", audio_ref_clock);
                //根据主时钟计算音频视频延迟
                audio_video_delay = videoPicture->pts - audio_ref_clock;

                if (_DEBUG_)SDL_Log("Audio Video Delay:\t\t%f\n", audio_video_delay);
                // 考虑考延迟的关系，因此跳过重复的帧
                sync_threshold = (pts_delay > AV_SYNC_THRESHOLD) ? pts_delay : AV_SYNC_THRESHOLD;

                if (_DEBUG_)SDL_Log("Sync Threshold:\t\t\t%f\n", sync_threshold);
                //计算视频帧和音频帧之间差值和给定阈值的比较
                if(fabs(audio_video_delay) < AV_NOSYNC_THRESHOLD){
                    //音视频之间的差和视频帧之间的差比较，如果音视频之间的延迟大于同步阈值sync_threshold的话就要加大视频帧之间的延迟
                    if(audio_video_delay <= -sync_threshold){
                        pts_delay = 0;
                    }else if (audio_video_delay >= sync_threshold){
                        pts_delay = 2 * pts_delay;
                    }
                }
            }

            if (_DEBUG_)SDL_Log("Corrected PTS delay:\t%f\n", pts_delay);
            videoState->frame_timer += pts_delay; //累积帧与帧之间的播放延迟
            // 计算真实的延迟，将微秒转换为秒
            real_delay = videoState->frame_timer - (av_gettime() / 1000000.0);
            if (_DEBUG_)SDL_Log("Real Delay:\t\t\t\t%f\n", real_delay);

            if (real_delay < 0.010){
                real_delay = 0.010;
            }

            if (_DEBUG_)SDL_Log("Corrected Real Delay:\t%f\n", real_delay);
            schedule_refresh(videoState, (Uint32)(real_delay * 1000 + 0.5));

            if (_DEBUG_)SDL_Log("Next Scheduled Refresh:\t%f\n\n", (real_delay * 1000 + 0.5));
            video_display(videoState);

            // 更新读取下一帧的索引
            if(++videoState->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE){
                videoState->pictq_rindex = 0;
            }
            SDL_LockMutex(videoState->pictq_mutex);
            videoState->pictq_size--;
            SDL_CondSignal(videoState->pictq_cond);
            SDL_UnlockMutex(videoState->pictq_mutex);
        }
    }else{
        schedule_refresh(videoState, 100);
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

    // const char * fileName = "D:\\SoftwareFamily\\QT\\projects\\QtFFmpeg_Application\\QtSDLFFmpeg_videoPlayer_v1\\resources\\GuoYongZhe.mp4";
    const char * fileName = "D:\\SoftwareFamily\\QT\\projects\\QtVideoPlayer_v2\\resources\\videos\\GuoYongZhe.mp4";

    VideoState * videoState = NULL;
    videoState = (VideoState*)av_mallocz(sizeof(VideoState));

    //初始化videoState -> filename的文件路径
    SDL_memcpy(videoState -> filename,fileName,sizeof(videoState -> filename));
    SDL_Log("file path is %s",videoState -> filename);

    // 默认为AV_SYNC_AUDIO_MASTER
    videoState->av_sync_type = DEFAULT_AV_SYNC_TYPE;

    videoState -> pictq_mutex = SDL_CreateMutex();
    videoState -> pictq_cond = SDL_CreateCond();

    schedule_refresh(videoState,100);
    videoState -> decode_tid = SDL_CreateThread(decode_thread,"decodeThread",videoState);
    if(!videoState -> decode_tid){
        SDL_Log("create thread is failed and %s",SDL_GetError());
        av_free(videoState);
        SDL_DestroyCond(videoState -> pictq_cond);
        SDL_DestroyMutex(videoState -> pictq_mutex);
        SDL_Quit();
        return 0;
    }

    flush_pkt = av_packet_alloc();
    flush_pkt -> data = (uint8_t*)"FLUSH";

    SDL_Event event;
    while(true){
        double incr, pos;

        SDL_WaitEvent(&event);
        switch(event.type){
        case SDL_KEYDOWN:
            switch(event.key.keysym.sym){
            case SDLK_LEFT:{
                incr = -10.0;
                goto do_seek;
                break;
            }
            case SDLK_RIGHT:{
                incr = 10.0;
                goto do_seek;
                break;
            }
            case SDLK_DOWN:{
                incr = -60.0;
                goto do_seek;
                break;
            }
            case SDLK_UP:{
                incr = 60.0;
                goto do_seek;
                break;
            }
            do_seek:{
                if(global_video_state){
                    pos = get_master_clock(global_video_state);
                    pos += incr;//当前的音频或者视频时钟 + 移动的位置 = 移动之后的位置
                    //AV_TIME_BASE 定义了时间戳的基准，使得在处理音视频流时能够以一致的方式进行时间计算和转换。
                    stream_seek(global_video_state, (int64_t)(pos * AV_TIME_BASE), incr);
                }
                break;
            };

            default:{
                SDL_Log("left right down up is not do anything");
                break;
            }
            }
            break;
        case FF_QUIT_EVENT:
            videoState -> quit = 1;
            SDL_Quit();
            break;
        case SDL_QUIT:
            videoState->quit = 1;
            SDL_CondSignal(videoState->audioq.cond);
            SDL_CondSignal(videoState->videoq.cond);
            SDL_Quit();
            break;
        case SDL_WINDOWEVENT:
            if(event.window.event == SDL_WINDOWEVENT_CLOSE){
                if(event.window.windowID == SDL_GetWindowID(screen)){
                    SDL_DestroyWindow(screen);
                    videoState -> quit = 1;
                    SDL_Quit();
                }
            }
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
    av_packet_unref(flush_pkt);
    SDL_Quit();
    return 0;
}
