#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

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

struct Button {
    SDL_Rect rect;
    SDL_Color color;
    Button(int x,int y,int w,int h,Uint8 r,Uint8 g,Uint8 b,Uint8 a){
        this -> rect.x = x;
        this -> rect.y = y;
        this -> rect.w = w;
        this -> rect.h = h;
        this -> color.r = r;
        this -> color.g = g;
        this -> color.b = b;
        this -> color.a = a;
    }
};
//检测鼠标点击的范围是否再按钮的给定区域
int isMouseOverButton(SDL_Rect rect,int mouseX,int mouseY){
    return (mouseX > rect.x && mouseX < rect.x + rect.w &&
            mouseY > rect.y && mouseY < rect.y + rect.h);
}

TTF_Font * initTTF(){
    int ret = TTF_Init();
    if(ret == -1){
        SDL_Log("TTF init is failed %s",TTF_GetError());
        return NULL;
    }
    TTF_Font * font = TTF_OpenFont("C:\\Windows\\Fonts\\times.ttf",24);
    return font;
}

// 绘制文本
SDL_Texture* loadTextTexture(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) {
        SDL_Log("Unable to create text surface! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return NULL;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if(texture == NULL){
        SDL_Log("from SDL_Surface to SDL_Texture is failed %s",SDL_GetError());
        return NULL;
    }
    SDL_FreeSurface(surface);
    return texture;
}

//获取文本高宽
SDL_Rect getTextureSize(SDL_Texture * textTexture,SDL_Rect textRect){
    SDL_Rect textDstRect;  // 文本目标矩形

    // 获取文本的宽高
    int textWidth, textHeight;
    SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);
    textDstRect = { textRect.x + (textRect.w - textWidth) / 2,
                   textRect.y + (textRect.h - textHeight) / 2,
                   textWidth, textHeight };
    return textDstRect;
}
//打开文件路径
QString getFilePath(){
    QString path = QFileDialog::getOpenFileName(
        nullptr,QString("打开文件"),".","All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)");
    if(path.isEmpty()){
        SDL_Log("打开文件失败");
        return "";
    }
    QFileInfo fileinformation(path);
    return path;
}


#include <QQueue>

struct userData {
    Uint8 * audio_buf;
    Uint32 audio_len;
    int curr_len;
};


struct structVideoPlayer{
    // 创建 SDL 窗口
    SDL_Window * window;
    SDL_Renderer * renderer;
    SDL_Texture * texture;
    SDL_mutex * mutex;
    SDL_AudioSpec audio_spec;


    AVFormatContext * pFormatCtx = nullptr;
    const AVCodec * pCodec = nullptr;
    AVCodecContext * pCodecCtx = nullptr;
    AVCodecContext * audio_codec_ctx = nullptr;

    int videoStreamIndex;
    int audioStreamIndex;

    int width;
    int height;

    userData * userdata = new userData;
    SDL_cond * cond;

    SDL_Rect playWin;
    QQueue<AVPacket>audio_queue;
    SDL_AudioDeviceID device_id;//记录当前打开的音频设备ID
    std::mutex c_mutex;
    std::condition_variable cv;
    // std::queue<AVPacket>audio_queue;

    structVideoPlayer(){
        this -> window = nullptr;
        this -> renderer = nullptr;
        this -> texture = nullptr;
        this -> mutex = nullptr;

        this -> pFormatCtx = nullptr;
        this -> pCodec = nullptr;
        this -> pCodecCtx = nullptr;
        this -> audio_codec_ctx = nullptr;

        this -> cond = nullptr;

        this -> videoStreamIndex = -1;
        this -> audioStreamIndex = -1;
        this -> width = 0;
        this -> height = 0;

        this -> playWin = {0,0,0,0};

        this -> userdata -> audio_buf = nullptr;
        this -> userdata -> audio_len = 0;
        this -> userdata -> curr_len = 0;
    }
};

void audioCallback(void *data, Uint8 * stream,int len){
    structVideoPlayer * vip = (structVideoPlayer*)data;

    std::unique_lock<std::mutex> lock(vip -> c_mutex);

    SDL_memset(stream, 0, len);
    if(vip -> userdata -> audio_len <= 0){
        return;
    }
    //通过输出，发现实际读取的音频数据长度小于len = sample * 16 / 8 * channels
    // qDebug()<<"callback len = "<<len;
    while(vip -> userdata -> audio_len > 0){
        if(vip -> userdata -> audio_len < (Uint32)len){
            len = vip -> userdata -> audio_len;
            // SDL_MixAudio(stream,vip -> userdata -> audio_buf + vip -> userdata ->curr_len,len,SDL_MIX_MAXVOLUME);
            SDL_memcpy(stream,vip -> userdata -> audio_buf + vip -> userdata ->curr_len,len);
        }else{
            // SDL_MixAudio(stream,vip -> userdata ->audio_buf + vip -> userdata ->curr_len,len,SDL_MIX_MAXVOLUME);
            SDL_memcpy(stream,vip -> userdata -> audio_buf + vip -> userdata ->curr_len,len);
        }
        vip -> userdata -> audio_len -= len;
        vip -> userdata -> curr_len += len;
    }
}


void initStructPlayer(structVideoPlayer * vip,QString path){
    SDL_Log(".............init struct video palyer.............");
    // 分配上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    const char * filePath = path.toLocal8Bit().constData();

    //打开输入流和读取头部信息
    avformat_open_input(&pFormatCtx, filePath, nullptr, nullptr);
    //读取媒体文件得到流信息
    avformat_find_stream_info(pFormatCtx, nullptr);

    // 找到视频流
    const AVCodec * pCodec = nullptr;
    AVCodecContext * pCodecCtx = nullptr;
    // 找到视频流
    AVCodecContext * audio_codec_ctx = nullptr;

    int videoStreamIndex = -1;
    int audioStreamIndex = -1;

    //遍历上下文流
    for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++) {
        //如果当前流类型和指定的类型相等就退出
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIndex == -1) {
            videoStreamIndex = i;
            //找到一个与ID匹配的并且已经注册的解码器
            pCodec = avcodec_find_decoder(pFormatCtx->streams[i]->codecpar->codec_id);
            //分配AVCodecContext并将其字段设置为默认值
            //以存储与找到的解码器相关的信息。这个上下文将用于后续的解码操作。
            pCodecCtx = avcodec_alloc_context3(pCodec);
            //根据提供的编解码器参数中的值填充编解码器上下文。
            //根据当前流的编码参数填充 AVCodecContext。这包括解析度、比特率、帧率等重要信息，使解码器知道如何正确解码该视频流。
            avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[i]->codecpar);
            //根据解码器对上下文进行初始化;将所选解码器与上下文关联，以便可以开始解码过程。
            avcodec_open2(pCodecCtx, pCodec, nullptr);
        }

        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIndex == -1) {
            audioStreamIndex = i;
            const AVCodec *audio_codec = avcodec_find_decoder(pFormatCtx->streams[i]->codecpar->codec_id);
            audio_codec_ctx = avcodec_alloc_context3(audio_codec);
            avcodec_parameters_to_context(audio_codec_ctx, pFormatCtx->streams[i]->codecpar);
            avcodec_open2(audio_codec_ctx, audio_codec, nullptr);
        }
    }

    SDL_Log("video stream index is %d",videoStreamIndex);
    SDL_Log("audio stream index is %d",audioStreamIndex);

    vip -> pCodec = pCodec;
    vip -> pCodecCtx = pCodecCtx;
    vip -> videoStreamIndex = videoStreamIndex;
    vip -> audioStreamIndex = audioStreamIndex;

    // SDL 音频初始化
    SDL_AudioSpec audio_spec;
    audio_spec.freq = audio_codec_ctx -> sample_rate;
    audio_spec.format = AUDIO_S16LSB; // 设置适当的格式(格式设置很重要)
    audio_spec.channels = audio_codec_ctx -> ch_layout.nb_channels;
    audio_spec.samples = 1024;
    audio_spec.callback = audioCallback;
    audio_spec.userdata = vip;

    vip -> audio_spec = audio_spec;
    vip -> pFormatCtx = pFormatCtx;
    vip -> audio_codec_ctx = audio_codec_ctx;
    vip -> audioStreamIndex = audioStreamIndex;

    SDL_Log("sampe rate = %d  channles = %d",audio_codec_ctx -> sample_rate,audio_codec_ctx -> ch_layout.nb_channels);

    vip -> mutex = SDL_CreateMutex();
    if(vip -> mutex == NULL){
        SDL_Log("create mutex is failed and %s",SDL_GetError());

        if(vip -> texture)SDL_DestroyTexture(vip -> texture);
        if(vip -> renderer)SDL_DestroyRenderer(vip -> renderer);
        if(vip -> window) SDL_DestroyWindow(vip -> window);
        SDL_CloseAudio();
        delete vip -> userdata;
        if(vip -> pCodecCtx)avcodec_free_context(&vip -> pCodecCtx);
        if(vip -> pFormatCtx)avformat_close_input(&vip -> pFormatCtx);
        if(vip -> audio_codec_ctx)avcodec_free_context(&vip -> audio_codec_ctx);
        SDL_Quit();
    }

    vip -> cond = SDL_CreateCond();

}

int videoPlayer(void * data){
    structVideoPlayer * vip = (structVideoPlayer*)data;

    SDL_Log("thread function videoPlayer");

    AVPacket packet;
    //分配一个AVFrame并将其字段设置为默认值。
    AVFrame *pFrame = av_frame_alloc();
    AVFrame *pFrameRGB = av_frame_alloc();

    SDL_Log(".......................................");
    SDL_Log("width and height = (%d  %d)",vip -> pCodecCtx -> width,vip -> pCodecCtx -> height);

    uint8_t *video_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                                                           vip -> pCodecCtx -> width,
                                                                           vip -> pCodecCtx -> height, 1));
    //分配缓冲区并在一次调用中填充dst_data和dst_linesize
    int ret = av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, video_buffer, AV_PIX_FMT_YUV420P,
                                   vip -> pCodecCtx -> width, vip -> pCodecCtx -> height, 1);
    if(ret < 0){
        SDL_Log("av image fill arrays is failed");
        avcodec_free_context(&vip -> pCodecCtx);
        SDL_DestroyRenderer(vip -> renderer);
        SDL_DestroyTexture(vip -> texture);
        SDL_DestroyWindow(vip -> window);
        SDL_Quit();
        return 0;
    }

    //分配和返回SwsContext，用于在sws_scale中执行对图像的缩放以及转换操作
    struct SwsContext *sws_ctx = sws_getContext(vip -> pCodecCtx -> width, vip -> pCodecCtx -> height,
                                                vip -> pCodecCtx->pix_fmt,vip -> pCodecCtx -> width,
                                                vip -> pCodecCtx -> height, AV_PIX_FMT_YUV420P,
                                                SWS_BILINEAR, nullptr, nullptr, nullptr);

    //执行读取视频帧并显示
    while (av_read_frame(vip -> pFormatCtx, &packet) >= 0) {//读取帧并将信息存储到packet中
        //首先判断当前读取的数据流和指定的视频流编号是否相同
        if (packet.stream_index == vip -> videoStreamIndex) {

            //提供了录制、转换以及流化音频和视频的能力。在编码过程中，avcodec_send_packet 函数将原始数据包（如从文件、
            //网络或摄像头捕获的帧）发送到编码器以供处理的角色。
            avcodec_send_packet(vip -> pCodecCtx, &packet);
            //获取编码后的数据
            while (avcodec_receive_frame(vip -> pCodecCtx, pFrame) >= 0) {
                //在srcSlice中缩放图像切片，并将缩放后的切片放入dst中的图像中。切片是图像中连续行的序列。
                //将pFrame图像切片缩放之后存储到pFrameRGB中;返回图像切片的高度
                sws_scale(sws_ctx, pFrame->data, pFrame->linesize,
                                       0, vip -> pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
                // SDL_Log("sws scale return is %d",height);

                SDL_UpdateTexture(vip -> texture, nullptr, pFrameRGB->data[0], pFrameRGB->linesize[0]);
                // SDL_Log("update texture is failed and %s",SDL_GetError());

                SDL_RenderClear(vip -> renderer);//清除渲染器缓冲区的内容
                SDL_RenderCopy(vip -> renderer, vip -> texture, nullptr, nullptr);
                SDL_RenderPresent(vip -> renderer);
            }
            //每一次的视频帧更新之后都发送一个信号，音频播放线程被激活
            SDL_CondSignal(vip -> cond);
            SDL_Delay(30);
        }
        //用于释放和重置 AVPacket 结构体
        av_packet_unref(&packet);
    }

    // 清理资源
    av_free(video_buffer);
    av_frame_free(&pFrameRGB);
    av_frame_free(&pFrame);
    sws_freeContext(sws_ctx);
    SDL_Log("video thread is end......");
    //当播放视频的线程结束之后，但是实际上音频播方并没有完成，因此还需要发送一个信号，唤醒音频播放线程
    SDL_CondSignal(vip -> cond);
    return 0;
}

int audioPlayer(void * data){
    structVideoPlayer * vip = (structVideoPlayer*)data;
    SDL_Log("thread function audioPlayer");

    //打开音频设备
    // vip -> device_id = SDL_OpenAudioDevice(nullptr,0,&vip -> audio_spec,nullptr,0);
    // if(vip -> device_id == 0){
    //     SDL_Log("open audio device is failed and %s",SDL_GetError());
    //     SDL_DestroyTexture(vip -> texture);
    //     SDL_DestroyRenderer(vip -> renderer);
    //     SDL_DestroyWindow(vip -> window);
    //     delete userdata;
    //     if(vip -> pCodecCtx)avcodec_free_context(&vip -> pCodecCtx);;
    //     if(vip -> pFormatCtx)avformat_close_input(&vip -> pFormatCtx);
    //     if(vip -> audio_codec_ctx)avcodec_free_context(&vip -> audio_codec_ctx);
    //     SDL_Quit();
    //     return 0;
    // }

    // SDL_PauseAudioDevice(vip -> device_id,0);// 启动音频播放

    SDL_OpenAudio(&vip -> audio_spec,nullptr);
    SDL_PauseAudio(0);

    //音频采样
    AVPacket packet;
    enum AVSampleFormat sample_fmt = AV_SAMPLE_FMT_S16;
    AVFrame * audioFrame = av_frame_alloc();


    int size = av_samples_get_buffer_size(audioFrame -> linesize,
                                          vip -> audio_spec.channels,
                                          vip -> audio_spec.samples,sample_fmt,0);

    SDL_Log("sample buffer size is %d",size);
    uint8_t * audio_buffer = (uint8_t * )av_malloc(size);
    int ret = av_samples_fill_arrays(audioFrame -> data,audioFrame -> linesize,audio_buffer,
                                     vip -> audio_spec.channels,vip -> audio_spec.samples,
                                     sample_fmt,0);

    if(ret < 0){
        SDL_Log("av samples fill arrays is failed");
        avcodec_free_context(&vip -> pCodecCtx);
        avcodec_free_context(&vip -> audio_codec_ctx);
        SDL_DestroyRenderer(vip -> renderer);
        SDL_DestroyTexture(vip -> texture);
        SDL_DestroyWindow(vip -> window);
        delete vip -> userdata;
        SDL_Quit();
        return 0;
    }

    struct SwrContext * convert_context = swr_alloc();
    AVChannelLayout channelLayout;
    av_channel_layout_default(&channelLayout,2);

    ret = swr_alloc_set_opts2(&convert_context,&channelLayout,AV_SAMPLE_FMT_S16,44100,
                              &channelLayout,vip -> audio_codec_ctx -> sample_fmt,
                              vip -> audio_codec_ctx -> sample_rate,0,nullptr);
    if(ret < 0){
        SDL_Log("set/reset parameters is failed");
        avcodec_free_context(&vip -> pCodecCtx);
        avcodec_free_context(&vip -> audio_codec_ctx);
        SDL_DestroyRenderer(vip -> renderer);
        SDL_DestroyTexture(vip -> texture);
        SDL_DestroyWindow(vip -> window);
        delete vip -> userdata;
        SDL_Quit();
        return 0;
    }

    swr_init(convert_context);//注意不要忽略这一句：初始化 SWR 上下文

    int out_samples_max = av_rescale_rnd(
                        swr_get_delay(convert_context, vip -> audio_codec_ctx -> sample_rate)
                        + vip -> audio_spec.samples,44100, vip -> audio_codec_ctx -> sample_rate, AV_ROUND_UP);
    SDL_Log("out samples max is %d",out_samples_max);

    while (av_read_frame(vip -> pFormatCtx, &packet) >= 0) {

        if(packet.stream_index == vip -> audioStreamIndex){
            ret = SDL_LockMutex(vip -> mutex);
            if(ret == 0){

                SDL_CondWait(vip -> cond,vip -> mutex);
                //将原始的音频数据发送到解码器进行解码；调用将复制相关的AVCodecContext字段，
                //这可以影响每个数据包的解码，并在数据包实际解码时应用它们。
                avcodec_send_packet(vip -> audio_codec_ctx, &packet);
                //获取解码（或者编码）后的音频数据
                while(avcodec_receive_frame(vip -> audio_codec_ctx,audioFrame) >= 0){
                    int per_channel_samples = swr_convert(convert_context,(uint8_t**)&audio_buffer,
                                                          out_samples_max,
                                                          (const uint8_t **) audioFrame -> data,
                                                          vip -> audio_spec.samples);
                    if(per_channel_samples > 0)SDL_Log("per channel samples is %d",per_channel_samples);
                    else {
                        SDL_Log("compute per channel samples is failed");
                        avcodec_free_context(&vip -> pCodecCtx);
                        avcodec_free_context(&vip -> audio_codec_ctx);
                        SDL_DestroyRenderer(vip -> renderer);
                        SDL_DestroyTexture(vip -> texture);
                        SDL_DestroyWindow(vip -> window);
                        delete vip -> userdata;
                        av_free(audio_buffer);
                        av_frame_free(&audioFrame);
                        if(convert_context != nullptr)swr_free(&convert_context);
                        SDL_Quit();
                        return 0;
                    }

                    //如果还没有播放完毕，就等待继续播放(其实在回调函数中已经采用了while循环来实现这一点)
                    while(vip -> userdata -> audio_len > 0){
                        SDL_Delay(1);
                    }
                    if (vip->userdata->audio_buf) {
                        free(vip->userdata->audio_buf); // 释放之前的缓冲区
                    }
                    vip -> userdata -> audio_buf = (uint8_t*)malloc(size);
                    SDL_memcpy(vip -> userdata -> audio_buf,audio_buffer,size);
                    vip -> userdata -> audio_len = size;
                    vip -> userdata -> curr_len = 0;
                }
                SDL_Delay(10);
                SDL_UnlockMutex(vip -> mutex);
            }else{
                SDL_Log("lock mutex is failed and %s",SDL_GetError());

                SDL_DestroyTexture(vip -> texture);
                SDL_DestroyRenderer(vip -> renderer);
                SDL_DestroyWindow(vip -> window);
                if(vip -> pCodecCtx)avcodec_free_context(&vip -> pCodecCtx);;
                if(vip -> pFormatCtx)avformat_close_input(&vip -> pFormatCtx);
                if(vip -> audio_codec_ctx)avcodec_free_context(&vip -> audio_codec_ctx);
                if(convert_context != nullptr)swr_free(&convert_context);
            }
        }
        //用于释放和重置 AVPacket 结构体
        av_packet_unref(&packet);
    }
    av_free(audio_buffer);
    av_frame_free(&audioFrame);
    if(convert_context != nullptr)swr_free(&convert_context);

    SDL_Log("audio thread is end......");

    return 0;
}

void windowPlayer(structVideoPlayer * vip){
    // 创建 SDL 窗口
    SDL_Window *window = nullptr;
    SDL_Renderer * renderer = nullptr;

    // int ret = SDL_CreateWindowAndRenderer(vip -> pCodecCtx -> width, vip -> pCodecCtx -> height,
    //                                       SDL_WINDOW_MOUSE_CAPTURE, &window, &renderer);
    // if(ret == -1){
    //     SDL_Log("initialize window and renderer is failed and %s",SDL_GetError());
    //     delete vip -> userdata;
    //     SDL_Quit();
    //     return;
    // }
    // 获取当前视频显示
    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        SDL_Log("SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());
        SDL_Quit();
        return ;
    }
    // 计算窗口应该放置的x和y坐标，以使其居中 ；这里之所以分别创建窗口和渲染器，主要是因为方便关闭窗口
    int windowWidth = vip -> pCodecCtx -> width;
    int windowHeight = vip -> pCodecCtx -> height;
    int screenWidth = dm.w;
    int screenHeight = dm.h;
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    window = SDL_CreateWindow("playing",x,y,windowWidth,windowHeight,0);
    renderer = SDL_CreateRenderer(window,-1,0);
    if(renderer == nullptr){
        SDL_Log("initialize window and renderer is failed and %s",SDL_GetError());
        SDL_Quit();
        return ;
    }

    //创建用于渲染上下文的纹理
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
                                             vip -> pCodecCtx -> width, vip -> pCodecCtx -> height);
    vip -> window = window;
    vip -> renderer = renderer;
    vip -> texture = texture;
    vip -> width = vip -> pCodecCtx -> width;
    vip -> height = vip -> pCodecCtx -> height;
    //防止主线程阻塞，采用SDL中的多线程机制;其中videoPlayer用于唤起另外一个线程
    SDL_Thread * video_thread = SDL_CreateThread(videoPlayer,"videoThread",vip);
    if (video_thread == NULL) {
        SDL_Log("Could not create thread: %s\n", SDL_GetError());
    }

    SDL_Thread * audio_thread = SDL_CreateThread(audioPlayer,"audioThread",vip);
    if (audio_thread == NULL) {
        SDL_Log("Could not create thread: %s\n", SDL_GetError());
    }

    bool isRunning = true;
    SDL_Event e;
    while(isRunning){
        while(SDL_PollEvent(&e)){//用于从事件队列中获取待处理的事件
            if(e.type == SDL_QUIT){
                isRunning = false;
            }else if(e.type == SDL_KEYDOWN){
                switch(e.key.keysym.sym){
                case SDLK_ESCAPE://按下ESC键，退出程序
                    isRunning = false;
                }
            }else if(e.type == SDL_WINDOWEVENT){
                if(e.window.event == SDL_WINDOWEVENT_CLOSE){
                    if(e.window.windowID == SDL_GetWindowID(window)){
                        SDL_DestroyWindow(window);
                        SDL_DestroyRenderer(renderer);
                        SDL_DestroyTexture(texture);
                        window = nullptr;
                        renderer = nullptr;
                        texture = nullptr;
                        isRunning = false;
                    }
                }
            }
        }
        SDL_Delay(10);
    }

    //返回指定线程的ID
    SDL_threadID video_threadID = SDL_GetThreadID(video_thread);
    SDL_Log("current video thread id is %d",(int)video_threadID);
    SDL_threadID audio_threadID = SDL_GetThreadID(audio_thread);
    SDL_Log("current audio thread id is %d",(int)audio_threadID);

    //等待线程结束
    int status1 = 0,status2 = 0;
    if(video_thread)SDL_WaitThread(video_thread,&status1);
    if(audio_thread)SDL_WaitThread(audio_thread,&status2);
    if(status1 == 0 && status2 == 0){//线程正常退出
        SDL_Log("The video and audio thread is finished !");//线程正常退出
    }else{
        SDL_Log("The video or audio thread is abnormal exit !");//异常退出
    }

    if(renderer)SDL_DestroyRenderer(renderer);
    if(texture)SDL_DestroyTexture(texture);
    if(window)SDL_DestroyWindow(window);
    // SDL_Quit();
    return;
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

    structVideoPlayer vip;
    QString path;

    // 创建 SDL 窗口
    SDL_Window *window = nullptr;
    SDL_Renderer * renderer = nullptr;

    // ret = SDL_CreateWindowAndRenderer(800, 600, 0, &window, &renderer);
    // 获取当前视频显示
    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        SDL_Log("SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // 计算窗口应该放置的x和y坐标，以使其居中 ；这里之所以分别创建窗口和渲染器，主要是因为方便关闭窗口
    int windowWidth = 800;
    int windowHeight = 600;
    int screenWidth = dm.w;
    int screenHeight = dm.h;
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    window = SDL_CreateWindow("video player",x,y,windowWidth,windowHeight,0);
    renderer = SDL_CreateRenderer(window,-1,0);
    if(renderer == nullptr){
        SDL_Log("initialize window and renderer is failed and %s",SDL_GetError());
        SDL_Quit();
        return 0;
    }

    //创建用于渲染上下文的纹理
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
                                             800, 600);

    SDL_version v;
    SDL_VERSION(&v);
    qDebug()<<"SDL2 version: "<<v.major<<" "<<v.minor<<" "<<v.patch;
    qDebug()<<"ffmpeg version: "<<av_version_info();

    //创建一个用于显示内容的矩阵框
    SDL_Rect imgShow {20,20,600,450};

    vip.width = imgShow.w;
    vip.height = imgShow.h;
    vip.playWin = imgShow;

    QString title = QString("This is a video player");
    SDL_SetWindowTitle(vip.window,title.toStdString().c_str());

    //初始化和加载字体
    TTF_Font * font = initTTF();
    if(font == NULL){
        SDL_Log("TTF init is failed %s",TTF_GetError());
        SDL_DestroyRenderer(vip.renderer);
        SDL_DestroyWindow(vip.window);
        SDL_Quit();
        return 0;
    }

    //定义打开文件按钮
    Button * open = new Button(640,20,130,50,192,192,192,255);
    //定义播放按钮
    Button * player = new Button(640,120,130,50,192,192,192,255);
    //定义关闭窗口按钮
    Button * close = new Button(640,420,130,50,192,192,192,255);


    //给"打开文件按钮"绘制文本
    SDL_Color textColor = {0,0,0,255};
    const char * openText ="Open";
    SDL_Texture * openTexture = loadTextTexture(renderer,font,openText,textColor);
    //获取当前文本高宽
    SDL_Rect openTextRect = getTextureSize(openTexture,open -> rect);

    //给"打开文件按钮"绘制文本
    const char * playerText ="Player";
    SDL_Texture * playerTexture = loadTextTexture(renderer,font,playerText,textColor);
    //获取当前文本高宽
    SDL_Rect playerTextRect = getTextureSize(playerTexture,player -> rect);

    //给"打开文件按钮"绘制文本
    const char * closeText ="Close";
    SDL_Texture * closeTexture = loadTextTexture(renderer,font,closeText,textColor);
    //获取当前文本高宽
    SDL_Rect closeTextRect = getTextureSize(closeTexture,close -> rect);

    bool isRunning = true;
    SDL_Event e;
    while(isRunning){
        while(SDL_PollEvent(&e)){//用于从事件队列中获取待处理的事件
            if(e.type == SDL_QUIT){
                isRunning = false;
            }
            if(e.type == SDLK_DOWN){
                switch(e.key.keysym.sym){
                case SDLK_ESCAPE://按下ESC键，退出层序
                    isRunning = false;
                }
            }else if(e.type == SDL_MOUSEBUTTONDOWN){
                int x,y;
                SDL_GetMouseState(&x,&y);
                //判断当前坐标和给定几个按钮，如果点击了相应的按钮，即响应当前按钮
                if(isMouseOverButton(open -> rect,x,y)){
                    path = getFilePath();

                }else if(isMouseOverButton(player -> rect,x,y)){
                    //判断当前音频设备是否已经打开，如果已经打开就关闭
                    if((vip.device_id != 0) && (SDL_GetAudioDeviceStatus(vip.device_id) == SDL_AUDIO_PLAYING)){
                        SDL_PauseAudioDevice(vip.device_id,1);
                        SDL_CloseAudioDevice(vip.device_id);
                        SDL_Log("The audio %d is playing",vip.device_id);
                    }
                    //先释放之前申请的资源
                    if(vip.pCodecCtx)avcodec_free_context(&vip.pCodecCtx);;
                    if(vip.pFormatCtx)avformat_close_input(&vip.pFormatCtx);
                    if(vip.audio_codec_ctx)avcodec_free_context(&vip.audio_codec_ctx);

                    initStructPlayer(&vip,path);
                    SDL_CloseAudio();//如果音频设备没有关闭就先关闭
                    windowPlayer(&vip);

                }else if(isMouseOverButton(close -> rect,x,y)){
                    isRunning = false;
                }
            }
            int x,y;
            SDL_GetMouseState(&x,&y);
            //当鼠标移动到给定的按钮上面的时候就变为相应的颜色
            if(isMouseOverButton(open -> rect,x,y)){
                open -> color.r = 0;open -> color.g = 255;
                open -> color.b = 255;open -> color.a = 255;
            }else{
                open -> color.r = 192;open -> color.g = 192;
                open -> color.b = 192;open -> color.a = 255;
            }
            if(isMouseOverButton(player -> rect,x,y)){
                player -> color.r = 0;player -> color.g = 255;
                player -> color.b = 255;player -> color.a = 255;
            }else{
                player -> color.r = 192;player -> color.g = 192;
                player -> color.b = 192;player -> color.a = 255;
            }
            if(isMouseOverButton(close -> rect,x,y)){
                close -> color.r = 0;close -> color.g = 255;
                close -> color.b = 255;close -> color.a = 255;
            }else{
                close -> color.r = 192;close -> color.g = 192;
                close -> color.b = 192;close -> color.a = 255;
            }

            SDL_SetRenderDrawColor(renderer,open -> color.r,open -> color.g,open -> color.b,open->color.a);
            SDL_RenderFillRect(renderer,&open -> rect);

            SDL_SetRenderDrawColor(renderer,player -> color.r,player -> color.g,player -> color.b,player->color.a);
            SDL_RenderFillRect(renderer,&player -> rect);

            SDL_SetRenderDrawColor(renderer,close -> color.r,close -> color.g,close -> color.b,close->color.a);
            SDL_RenderFillRect(renderer,&close -> rect);

            SDL_SetRenderDrawColor(renderer,255,255,255,255);
            SDL_RenderFillRect(renderer,&imgShow);

            //给所有的按钮绘制文字
            SDL_RenderCopy(renderer, openTexture, NULL, &openTextRect);
            SDL_RenderCopy(renderer, playerTexture, NULL, &playerTextRect);
            SDL_RenderCopy(renderer, closeTexture, NULL, &closeTextRect);

            SDL_RenderPresent(renderer); //显示渲染器缓冲区的内容
        }
        SDL_Delay(10);
    }

    if((vip.device_id != 0) && (SDL_GetAudioDeviceStatus(vip.device_id) == SDL_AUDIO_PLAYING)){
        SDL_PauseAudioDevice(vip.device_id,1);
        SDL_CloseAudioDevice(vip.device_id);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);

    delete vip.userdata;
    delete open;delete player;delete close;
    if(vip.pCodecCtx)avcodec_free_context(&vip.pCodecCtx);;
    if(vip.pFormatCtx)avformat_close_input(&vip.pFormatCtx);
    if(vip.audio_codec_ctx)avcodec_free_context(&vip.audio_codec_ctx);
    SDL_Quit();

    return 0;
}
