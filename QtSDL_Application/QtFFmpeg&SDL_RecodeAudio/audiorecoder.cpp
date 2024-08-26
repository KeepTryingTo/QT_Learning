#include "audiorecoder.h"

AudioRecoder::AudioRecoder(QObject *parent):QThread(parent){
    this -> recording = false;
    //注册所有的设备，使得FFmpeg可以访问和使用这些设备。
    avdevice_register_all();
    //用于初始化网络功能，使得 FFmpeg 能够进行网络相关的操作，
    //如从网络流中读取数据或向网络流中写入数据。这个函数主要用于处理与 HTTP、RTSP、RTMP 等网络协议相关的音视频流。
    avformat_network_init();
}

void AudioRecoder::stopRecod(){
    this -> recording = false;
}

void AudioRecoder::run() {
    //用于在打开媒体文件或流之前，确定使用哪个输入格式
    //dshow 是 DirectShow 的缩写，它是微软 Windows 操作系统
    //中用于处理多媒体流的框架。dshow 作为一种输入格式，主要用来捕获音视频数据来源，如摄像头、音频设备、电视接收卡等。
    const AVInputFormat * fmt = av_find_input_format("dshow");

    AVFormatContext * ctx = nullptr;
    const char* audio_devices = "audio=麦克风阵列 (Realtek(R) Audio)";
    //打开输入设备
    avformat_open_input(&ctx,audio_devices,fmt,nullptr);

    //打开保存音频文件
    QFile file(this -> fileName);
    if(!file.open(QIODevice::WriteOnly)){
        qDebug()<<"打开文件失败";
        return;
    }
    AVPacket pkt;
    this -> recording = true;
    while(this -> recording && av_read_frame(ctx,&pkt) == 0){
        file.write((const char*)pkt.data,pkt.size);
    }
    file.close();
    //关闭上下文输入和释放相关的资源
    avformat_close_input(&ctx);
    avformat_free_context(ctx);
}
