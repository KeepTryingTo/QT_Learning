#include <SDL2/SDL.h>

#include <QDebug>
#include <QMessageBox>
#include <QString>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
}

#undef main

typedef struct videoPlayer{
    // 创建 SDL 窗口
    SDL_Window *window;
    SDL_Renderer * renderer;
    SDL_Texture *texture;
    AVFormatContext *pFormatCtx;
    const AVCodec * pCodec = nullptr;
    AVCodecContext * pCodecCtx = nullptr;
    int videoStreamIndex;

    videoPlayer(SDL_Window *window = nullptr,SDL_Renderer * renderer = nullptr){
        this -> window = window;
        this -> renderer = renderer;
    }
};

int videoPlayer(void * data){
    struct videoPlayer * vip = (struct videoPlayer*)data;

    AVPacket packet;
    //分配一个AVFrame并将其字段设置为默认值。
    AVFrame *pFrame = av_frame_alloc();
    AVFrame *pFrameRGB = av_frame_alloc();

    enum AVSampleFormat sample_fmt = AV_SAMPLE_FMT_FLTP; // 浮点格式

    uint8_t *buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, vip -> pCodecCtx->width,
                                                                     vip -> pCodecCtx->height, 1));
    //分配缓冲区并在一次调用中填充dst_data和dst_linesize
    int ret = av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_YUV420P,
                               vip -> pCodecCtx->width, vip -> pCodecCtx->height, 1);
    if(ret < 0){
        SDL_Log("av samples file arrays is failed");
        avcodec_free_context(&vip -> pCodecCtx);
        SDL_DestroyRenderer(vip -> renderer);
        SDL_DestroyTexture(vip -> texture);
        SDL_DestroyWindow(vip -> window);
        SDL_Quit();
        return 0;
    }

    //分配和返回SwsContext，用于在sws_scale中执行对图像的缩放以及转换操作
    struct SwsContext *sws_ctx = sws_getContext(vip -> pCodecCtx->width, vip -> pCodecCtx->height, vip -> pCodecCtx->pix_fmt,
                                                vip -> pCodecCtx->width, vip -> pCodecCtx->height, AV_PIX_FMT_YUV420P,
                                                SWS_BILINEAR, nullptr, nullptr, nullptr);

    //主线程执行读取视频帧并显示
    while (av_read_frame(vip -> pFormatCtx, &packet) >= 0) {//读取帧并将信息存储到packet中
        //首先判断当前读取的数据流和指定的视频流编号是否相同
        if (packet.stream_index == vip -> videoStreamIndex) {
            //提供了录制、转换以及流化音频和视频的能力。在编码过程中，avcodec_send_packet 函数扮演了将原始数据包（如从文件、
            //网络或摄像头捕获的帧）发送到编码器以供处理的角色。
            avcodec_send_packet(vip -> pCodecCtx, &packet);
            //获取编码后的数据
            while (avcodec_receive_frame(vip -> pCodecCtx, pFrame) >= 0) {
                //在srcSlice中缩放图像切片，并将缩放后的切片放入dst中的图像中。切片是图像中连续行的序列。
                //将pFrame图像切片缩放之后存储到pFrameRGB中;返回图像切片的高度
                int height = sws_scale(sws_ctx, pFrame->data, pFrame->linesize,
                                       0, vip -> pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
                // SDL_Log("sws scale return is %d",height);

                ret = SDL_UpdateTexture(vip -> texture, nullptr, pFrameRGB->data[0], pFrameRGB->linesize[0]);
                // SDL_Log("update texture is failed and %s",SDL_GetError());

                SDL_RenderClear(vip -> renderer);
                SDL_RenderCopy(vip -> renderer, vip -> texture, nullptr, nullptr);
                SDL_RenderPresent(vip -> renderer);
            }
            SDL_Delay(30);
        }
        //用于释放和重置 AVPacket 结构体
        av_packet_unref(&packet);
    }

    // 清理资源
    av_free(buffer);
    av_frame_free(&pFrameRGB);
    av_frame_free(&pFrame);
    sws_freeContext(sws_ctx);

    return 0;
}

int main(int argc, char *argv[]) {

    struct videoPlayer vip;

    // 初始化 SDL
    int ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    if(ret < 0){
        SDL_Log("initialize video is failed and %s",SDL_GetError());
        SDL_Quit();
        return 0;
    }

    avdevice_register_all();
    // 分配上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    const char * filePath = "D:\\SoftwareFamily\\QT\\projects\\QtVideoPlayer_v2\\resources\\videos\\GuoYongZhe.mp4";

    //打开输入流和读取头部信息
    avformat_open_input(&pFormatCtx, filePath, nullptr, nullptr);
    avformat_find_stream_info(pFormatCtx, nullptr);

    // 找到视频流
    const AVCodec *pCodec = nullptr;
    AVCodecContext *pCodecCtx = nullptr;
    int videoStreamIndex = -1;

    //遍历上下文流
    for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++) {
        //如果当前流类型和指定的类型相等就退出
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            //找到一个与ID匹配的并且已经注册的解码器
            pCodec = avcodec_find_decoder(pFormatCtx->streams[i]->codecpar->codec_id);
            //分配AVCodecContext并将其字段设置为默认值
            pCodecCtx = avcodec_alloc_context3(pCodec);
            //根据提供的编解码器参数中的值填充编解码器上下文。
            avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[i]->codecpar);
            //根据解码器对上下文进行初始化
            avcodec_open2(pCodecCtx, pCodec, nullptr);
            break;
        }
    }
    SDL_Log("video stream index is %d",videoStreamIndex);

    // 创建 SDL 窗口
    SDL_Window *window = nullptr;
    SDL_Renderer * renderer = nullptr;

    ret = SDL_CreateWindowAndRenderer(pCodecCtx -> width,pCodecCtx -> height, 0,&window,&renderer);
    if(ret == -1){
        SDL_Log("initialize window and renderer is failed and %s",SDL_GetError());
        SDL_Quit();
        return 0;
    }

    //创建用于渲染上下文的纹理
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STREAMING,
                                             pCodecCtx -> width,pCodecCtx -> height);

    vip.window = window;
    vip.renderer = renderer;
    vip.texture = texture;
    vip.pCodec = pCodec;
    vip.pFormatCtx = pFormatCtx;
    vip.pCodecCtx = pCodecCtx;
    vip.videoStreamIndex = videoStreamIndex;
    //防止主线程阻塞，采用SDL中的多线程机制;其中videoPlayer用于唤起另外一个线程
    SDL_Thread* thread = SDL_CreateThread(videoPlayer,"videoThread",&vip);
    if (thread == NULL) {
        SDL_Log("Could not create thread: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    bool isRunning = true;
    SDL_Event e;
    while(isRunning){
        while(SDL_PollEvent(&e)){
            if(e.type == SDL_QUIT){
                isRunning = false;
                break;
            }
            if(e.type == SDLK_DOWN){
                switch(e.key.keysym.sym){
                case SDLK_ESCAPE://按下ESC键，退出层序
                    isRunning = false;
                    break;
                }
            }
        }
    }

    //等待线程结束
    SDL_WaitThread(thread,nullptr);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
    SDL_Quit();

    return 0;
}
