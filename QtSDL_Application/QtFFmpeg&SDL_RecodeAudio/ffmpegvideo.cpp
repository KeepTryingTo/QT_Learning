#include "ffmpegvideo.h"
#include "ui_ffmpegvideo.h"

#include <iostream>
#include <fstream>
using namespace std;

FFmpegVideo::FFmpegVideo(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::FFmpegVideo)
{
    ui->setupUi(this);

    QFile file;
    file.setFileName(":/new/prefix1/resources/css/qthttpcss.css");
    file.open(QIODevice::ReadOnly);

    QString strCss = file.readAll();
    this -> setStyleSheet(strCss);

    this -> setWindowTitle("音频录音和播放");
    this -> setFixedSize(QSize(528,378));

    qDebug()<<av_version_info();

    avdevice_register_all();
    this -> getAllDevices();

    this -> recoder = new AudioRecoder;
    this -> initTabel();
    this -> initMixer();

    connect(ui -> btn_start,&QPushButton::clicked,this,&FFmpegVideo::startRecord);
    connect(ui -> btn_stop,&QPushButton::clicked,this,&FFmpegVideo::stopRecord);

    //当窗口关闭之后就释放线程资源
    connect(this,&QMainWindow::destroyed,[=](){
        this -> recoder -> exit();
        this -> recoder -> wait();
        this -> recoder -> deleteLater();
    });

#if 0
    列出所有可用的视频和音频捕获设备的名称命令： ffmpeg -list_devices true -f dshow -i dummy
    播放PCM文件命令： ffplay -f s16le -ar 44100 -i output.pcm
#endif
}

FFmpegVideo::~FFmpegVideo()
{
    delete ui;
}

void FFmpegVideo::initTabel(){
    //更新当前好友和在线列表
    ui -> tableWidget -> clear();//首先把之前的用户清单清除之后再重新更新用户列表
    //设置表的行和列大小
    ui -> tableWidget -> setColumnCount(1);
    ui -> tableWidget -> setRowCount(10);

    QStringList headerLabel;
    headerLabel << "录音显示列表";
    ui -> tableWidget -> setHorizontalHeaderLabels(headerLabel);
    //设置表头伸展模式
    ui -> tableWidget -> horizontalHeader() -> setSectionResizeMode(QHeaderView::Stretch);

    // 访问水平表头
    QHeaderView *header = ui -> tableWidget->horizontalHeader();
    // 创建一个字体对象，并将其设置为加粗
    QFont font = header->font();
    font.setBold(true);
    header->setFont(font);
}

//初始化mixer子系统
void FFmpegVideo::initMixer(){
    int ret = Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_WAVPACK);
    if(ret == -1){
        SDL_Log("initlize mixer is failed and %s",Mix_GetError());
        return;
    }
    //打开音频设备;chunksize: 以采样帧为单位的音频缓冲区大小(总采样数除以通道数)。
    ret = Mix_OpenAudio(MIX_DEFAULT_FREQUENCY,MIX_DEFAULT_FORMAT,MIX_DEFAULT_CHANNELS,4096);
    if(ret == -1){
        SDL_Log("mixer open audio device is failed and %s",Mix_GetError());
        return;
    }
}
//将打开的音频文件加入到列表中
void FFmpegVideo::addTableWidgetItem(QString filePath){
    //记录之前,首先要判断一下列表和映射表中是否已经存在相同文件名的路径
    for(const auto & p : this -> map.values()){
        if(p.contains(filePath)){
            return;
        }
    }
    this -> map[this -> rows] = filePath;

    QFileInfo info(filePath);
    qDebug()<<"filename = "<<info.fileName();
    QTableWidgetItem * keyItem = new QTableWidgetItem(info.fileName());
    ui -> tableWidget -> setColumnWidth(this -> rows,ui -> tableWidget -> width());
    ui -> tableWidget -> setItem(this -> rows,0,keyItem);
    this -> rows ++;
}

void FFmpegVideo::getAllDevices(){
    const AVInputFormat * audio_inputFormat = nullptr;
    ui -> plainTextEdit -> appendPlainText("all audio input devices: ");
    for(;(audio_inputFormat = av_input_audio_device_next(audio_inputFormat));){
        ui -> plainTextEdit -> appendPlainText(audio_inputFormat -> name);
    }
    ui -> plainTextEdit -> appendPlainText("---------------------------------");

    ui -> plainTextEdit -> appendPlainText("all audio output devices: ");
    const AVOutputFormat * audio_outputFormat = nullptr;
    for(;(audio_outputFormat = av_output_audio_device_next(audio_outputFormat));){
        ui -> plainTextEdit -> appendPlainText(audio_outputFormat -> name);
    }
    ui -> plainTextEdit -> appendPlainText("---------------------------------");

    ui -> plainTextEdit -> appendPlainText("all video input devices: ");
    const AVInputFormat * video_inputFormat = nullptr;
    for(;(video_inputFormat = av_input_video_device_next(video_inputFormat));){
        ui -> plainTextEdit -> appendPlainText(video_inputFormat -> name);
    }
    ui -> plainTextEdit -> appendPlainText("---------------------------------");

    ui -> plainTextEdit -> appendPlainText("all video output devices: ");
    const AVOutputFormat * video_outputFormat = nullptr;
    for(;(video_outputFormat = av_output_video_device_next(video_outputFormat));){
        ui -> plainTextEdit -> appendPlainText(video_outputFormat -> name);
    }
    ui -> plainTextEdit -> appendPlainText("---------------------------------");

    avformat_network_deinit();
}

void FFmpegVideo::on_btn_save_clicked(){
    this -> globalPath = QFileDialog::getExistingDirectory(this,tr("save"));
}

void FFmpegVideo::startRecord(){
    ui -> plainTextEdit -> appendPlainText("开始录制");
    this -> recoder -> fileName = this -> globalPath + "/" + "audio_" + QString::number(this -> rows) + ".pcm";
    this -> addTableWidgetItem(this -> recoder -> fileName);
    qDebug()<<"save path is: "<<this -> recoder -> fileName;
    this -> recoder -> start();
}

void FFmpegVideo::stopRecord(){
    this -> recoder -> stopRecod();
    SDL_Delay(3000);//保存文件之后延迟50毫秒
    ui -> plainTextEdit -> appendPlainText("关闭录音成功");
    ui -> plainTextEdit -> appendPlainText("---------------------------------");
}

void FFmpegVideo::on_pushButton_clicked(){
    this -> close();
}

void FFmpegVideo::updateDuration(qint64 Mduration){
    QTime totalTime((Mduration / 3600) % 60,(Mduration / 60) % 60,Mduration % 60,(Mduration * 1000) % 1000);
    QString format = "mm:ss";
    if(this -> Mduration > 3600){
        format = "hh:mm:ss";
    }
    ui -> plainTextEdit -> appendPlainText(totalTime.fromString(format).toString());
}

void FFmpegVideo::on_btn_player_clicked(){

    if(this -> musicPath == ""){
        int currentRow = ui -> tableWidget -> currentRow();
        musicPath = this -> map[currentRow];
    }
    //加载文件
    qDebug()<<"musicpath = "<<this -> musicPath;
    music = Mix_LoadMUS(this -> musicPath.toStdString().c_str());
    if(!music){
        SDL_Log("Failed to load file and SDL_mixer Error: %s\n", Mix_GetError());
        SDL_Quit();
        return;
    }

    // 获取总时长（秒）
    this -> Mduration = (qint64)Mix_MusicDuration(music);
    if (Mduration < 0) {
        SDL_Log("Failed to get music duration! SDL_mixer Error: %s\n", Mix_GetError());
        SDL_Quit();
        return;
    }

    if(!music){
        QMessageBox::information(this,"提示","请加载文件");
        return;
    }
    int ret = Mix_PlayMusic(music,0);//不进行循环播放
    if(ret == -1){
        SDL_Log("Failed to play music! SDL_mixer Error: %s\n", Mix_GetError());
    }
    this -> updateDuration(this -> Mduration);
}

void FFmpegVideo::on_btn_open_clicked(){
    QString filePath = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(filePath.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }

    QFileInfo fileinformation(filePath);
    ui -> plainTextEdit -> appendPlainText("播放音频: " + fileinformation.fileName());
    this -> addTableWidgetItem(filePath);

    //判断打开的文件是否为.wav或者mp3文件；如果为pcm文件，则需要进行转换
    if(filePath.split(".")[1] == "mp3" || filePath.split(".")[1] == "wav"){
        this -> musicPath = filePath;
    }
}

void FFmpegVideo::on_btn_pcmTowav_clicked(){
    int currentRow = ui -> tableWidget -> currentRow();
    QString pcmPath = this -> map[currentRow];
    QString wavPath = pcmPath.split(".")[0] + ".wav";

    //首先判断当前目录下是否已经存在了相同的文件名，如果存在就删除
    QDir dir(wavPath);
    if(dir.exists()){
        dir.remove(wavPath);
    }

    //将录制的PCM音频格式转换为MP3或者WAV格式（查看网上的写法）
    this -> pcm2wav = new pcmConvertWav(pcmPath,wavPath);
    this -> pcm2wav -> start();
    ui -> plainTextEdit -> appendPlainText( "Successfully converted PCM to WAV");
    this -> addTableWidgetItem(wavPath);
}

void FFmpegVideo::on_btn_pause_player_clicked(){
    this -> isPaused = !(this -> isPaused);
    if(this -> isPaused){
        Mix_PauseMusic();
        this -> currPosition = Mix_GetMusicPosition(this -> music);
    }else{
        Mix_PlayMusic(this -> music,0);
        Mix_SetMusicPosition(this -> currPosition);
    }

    ui -> plainTextEdit -> appendPlainText("播放暂停成功");
    ui -> plainTextEdit -> appendPlainText("---------------------------------");
}

