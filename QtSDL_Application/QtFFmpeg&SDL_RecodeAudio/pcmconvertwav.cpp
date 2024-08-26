#include "pcmconvertwav.h"

#include <fstream>

pcmConvertWav::pcmConvertWav(QString inputPath,QString outPath,QObject *parent)
    : QThread{parent},inputPath {inputPath},outputPath{outPath}
{}

void pcmConvertWav::run()
{
    // 输入和输出文件名
    const char * inputFileName = this -> inputPath.toLocal8Bit().constData();
    const char * outputFileName = this -> outputPath.toLocal8Bit().constData();

    qDebug()<<"pcmpath = "<<this -> inputPath;
    qDebug()<<"wavpath = "<<this -> outputPath;

    // 创建输出格式上下文
    AVFormatContext *formatCtx = nullptr;
    const AVOutputFormat * outputFormat = av_guess_format("wav",nullptr,nullptr);
    int ret = avformat_alloc_output_context2(&formatCtx, outputFormat, nullptr, outputFileName);
    if (ret < 0) {
        qDebug() << "Could not create output context\n";
        return ;
    }
    // 为媒体文件添加新流
    AVStream *audioStream = avformat_new_stream(formatCtx, nullptr);
    if (!audioStream) {
        qDebug() << "Failed to create new stream\n";
        return ;
    }
    // 创建编码上下文
    AVCodecContext *codecCtx = avcodec_alloc_context3(nullptr);
    if (!codecCtx) {
        qDebug() << "Failed to allocate codec context\n";
        return ;
    }

    // 设置编码参数
    codecCtx-> codec_id = AV_CODEC_ID_PCM_S16LE; // 音频编码格式
    codecCtx-> codec_type = AVMEDIA_TYPE_AUDIO;
    codecCtx-> sample_rate = 44100; // 采样率
    codecCtx-> bits_per_coded_sample = 16;
    AVChannelLayout ch_layout;
    ch_layout.nb_channels = 2;
    codecCtx -> ch_layout = ch_layout;

    // 将编码参数添加到流中
    avcodec_parameters_from_context(audioStream->codecpar, codecCtx);

    qDebug()<<"outputPath = "<<outputFileName;
    // 打开输出文件
    if (!(formatCtx->oformat->flags & AVFMT_NOFILE)) {
        //pb: I/O上下文
        if (avio_open(&formatCtx->pb, outputFileName, AVIO_FLAG_WRITE) < 0) {
            qDebug() << "Could not open output file\n";
            return;
        }
    }
    // 写文件头
    avformat_write_header(formatCtx, nullptr);
    // 打开输入 PCM 文件
    std::ifstream pcmFile(inputFileName, std::ios::binary);
    if (!pcmFile.is_open()) {
        qDebug() << "Could not open PCM input file\n";
        return;
    }
    // 读取数据并写入 WAV 文件
    const int bufferSize = 4096;
    uint8_t buffer[bufferSize];

    while (pcmFile.read(reinterpret_cast<char*>(buffer), bufferSize)) {
        int bytesRead = pcmFile.gcount();
        // 创建 AVPacket 并填充数据
        AVPacket packet;
        av_init_packet(&packet);
        packet.data = buffer;
        packet.size = bytesRead;

        // 写入流
        av_interleaved_write_frame(formatCtx, &packet);
    }

    // 刷新缓冲区并写入尾部
    pcmFile.close();
    av_write_trailer(formatCtx);

    // 清理
    if (!(formatCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_close(formatCtx->pb);
    }
    //释放资源
    avcodec_free_context(&codecCtx);
    avformat_free_context(formatCtx);
}
