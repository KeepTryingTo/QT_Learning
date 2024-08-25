#include "sdlaudioplayer.h"
#include "ui_sdlaudioplayer.h"

SDLAudioPlayer::SDLAudioPlayer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SDLAudioPlayer)
{
    ui->setupUi(this);

    this -> initSDL();
    this -> initMixer();

    QFile file;
    file.setFileName(":/new/prefix1/resources/css/qthttpcss.css");
    file.open(QIODevice::ReadOnly);

    QString strCss = file.readAll();
    this -> setStyleSheet(strCss);

    this -> setWindowTitle("音频播放器");
    this -> setFixedSize(QSize(556,369));

    ui -> btn_start -> setIcon(style() -> standardIcon(QStyle::SP_MediaPlay));
    ui -> btn_pause -> setIcon(style() -> standardIcon(QStyle::SP_MediaPause));
    ui -> btn_stop -> setIcon(style() -> standardIcon(QStyle::SP_MediaStop));
    ui -> btn_seek_backward -> setIcon(style() -> standardIcon(QStyle::SP_MediaSeekBackward));
    ui -> btn_seek_feedward -> setIcon(style() -> standardIcon(QStyle::SP_MediaSeekForward));
    ui -> btn_muted -> setIcon(style() -> standardIcon(QStyle::SP_MediaVolume));

    QImage image(":/new/prefix1/resources/KTG.jpg");
    image = image.scaled(QSize(331,191),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);//缩放到指定大小

    ui -> label_image -> setPixmap(QPixmap::fromImage(image));

    this -> initTabel();

    ui -> slider_volumn -> setMinimum(0);
    ui -> slider_volumn -> setMaximum(100);
    ui -> slider_volumn -> setValue(10);
}

SDLAudioPlayer::~SDLAudioPlayer()
{
    Mix_FreeMusic(music);
    Mix_CloseAudio();
    SDL_Quit();
    delete ui;
}

void SDLAudioPlayer::initTabel(){
    //更新当前好友和在线列表
    ui -> tableWidget -> clear();//首先把之前的用户清单清除之后再重新更新用户列表
    //设置表的行和列大小
    ui -> tableWidget -> setColumnCount(1);
    ui -> tableWidget -> setRowCount(10);

    QStringList headerLabel;
    headerLabel << "音频播放列表";
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

// 初始化SDL子系统
void SDLAudioPlayer::initSDL(){
    int ret = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    if(ret == -1){
        SDL_Log("initlize sdl is failed and %s",Mix_GetError());
        return;
    }
}

//初始化mixer子系统
void SDLAudioPlayer::initMixer(){
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

void SDLAudioPlayer::on_btn_close_clicked()
{
    this -> close();
}


void SDLAudioPlayer::on_btn_open_file_clicked()
{
    QString musicPath = QFileDialog::getOpenFileName(this,QString("打开文件"),".",
                                                     tr("MP3 Files(*.mp3);;MP4 Files(*.mp4);;All Files(*.*)"));
    if(musicPath.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    QFileInfo fileinformation(musicPath);
    this -> fileName = fileinformation.fileName();
    qDebug() <<"the audio path is: "<< musicPath;

    this -> addTableWidgetItem(musicPath);
}

//将打开的音频文件加入到列表中
void SDLAudioPlayer::addTableWidgetItem(QString filePath){
    this -> map[this -> rows] = filePath;

    QFileInfo info(filePath);
    qDebug()<<"filename = "<<info.fileName();
    QTableWidgetItem * keyItem = new QTableWidgetItem(info.fileName());
    ui -> tableWidget -> setColumnWidth(this -> rows,ui -> tableWidget -> width());
    ui -> tableWidget -> setItem(this -> rows,0,keyItem);
    this -> rows ++;
}

void SDLAudioPlayer::updateDuration(qint64 duration){
    QString timestr;
    if(duration || this -> Mduration){
        QTime CurrentTime((duration / 3600) % 60,(duration / 60) % 60,duration % 60,(duration * 1000) % 1000);
        QTime totalTime((Mduration / 3600) % 60,(Mduration / 60) % 60,Mduration % 60,(Mduration * 1000) % 1000);
        QString format = "mm:ss";
        if(this -> Mduration > 3600){
            format = "hh:mm:ss";
        }
        ui -> label_time_1 -> setText(CurrentTime.toString(format));
        ui -> label_time_2 -> setText(totalTime.toString(format));
    }
    if(duration)ui -> slider_time -> setValue(duration);
}

void SDLAudioPlayer::positionChanged(qint64 progress){
    if(!ui -> slider_time -> isSliderDown()){
        ui -> slider_time -> setValue(progress);
    }
    this -> updateDuration(progress);
}

void SDLAudioPlayer::on_btn_seek_backward_clicked(){
    if(!music){
        QMessageBox::information(this,"提示","请加载文件");
        return;
    }
    qint64 initTime = ui -> slider_time -> value();
    ui -> slider_time -> setValue(initTime - 10);
    Mix_SetMusicPosition(ui -> slider_time -> value());
}
void SDLAudioPlayer::on_btn_seek_feedward_clicked(){
    if(!music){
        QMessageBox::information(this,"提示","请加载文件");
        return;
    }
    qint64 initTime = ui -> slider_time -> value();
    ui -> slider_time -> setValue(initTime + 10);
    Mix_SetMusicPosition(ui -> slider_time -> value());
}

void SDLAudioPlayer::on_btn_stop_clicked(){
    Mix_PauseMusic();
}

void SDLAudioPlayer::on_btn_start_clicked(){
    //点击播放之前首先需要根据选中列表中的哪个音频文件进行播放

    int currentRow = ui -> tableWidget -> currentRow();
    QString musicPath = this -> map[currentRow];
    //加载文件
    music = Mix_LoadMUS(musicPath.toStdString().c_str());
    if(!music){
        SDL_Log("Failed to load file! SDL_mixer Error: %s\n", Mix_GetError());
    }

    // 获取总时长（秒）
    this -> Mduration = (qint64)Mix_MusicDuration(music);
    if (Mduration < 0) {
        SDL_Log("Failed to get music duration! SDL_mixer Error: %s\n", Mix_GetError());
    }
    //设置总的音频时长
    ui -> slider_time -> setMaximum(this -> Mduration);

    if(!music){
        QMessageBox::information(this,"提示","请加载文件");
        return;
    }
    int ret = Mix_PlayMusic(music,0);//不进行循环播放
    if(ret == -1){
        SDL_Log("Failed to play music! SDL_mixer Error: %s\n", Mix_GetError());
    }

    bool isRunning = true;
    while(isRunning){
        while(SDL_PollEvent(&e)){//用于从事件队列中获取待处理的事件
            if(e.type == SDL_QUIT){
                isRunning = false;
                return;
            }
            if(e.type == SDL_KEYDOWN){ //按下键盘响应事件
                switch(e.key.keysym.sym){
                case SDLK_ESCAPE://按下ESC键，退出层序
                    isRunning = false;
                    break;
                }
            }
        }
        double currPlayPosition = Mix_GetMusicPosition(music);
        updateDuration((qint64)currPlayPosition);
    }
}

void SDLAudioPlayer::on_btn_pause_clicked(){
    if(!music){
        QMessageBox::information(this,"提示","请加载文件");
        return;
    }
    //这个功能没有实现，暂时先留着
    isPaused = !isPaused;
    if(isPaused){
        Mix_PauseMusic();
        this -> currPlayPosition = Mix_GetMusicPosition(music);
    }else{
        Mix_PlayMusic(music,0);//不进行循环播放
        Mix_SetMusicPosition((double)(this -> currPlayPosition)); //根据上次暂停播放的位置进行设置
    }
}

void SDLAudioPlayer::on_btn_muted_clicked(){
    if(this -> is_muted == true){
        ui -> btn_muted -> setIcon(style() -> standardIcon(QStyle::SP_MediaVolumeMuted));
        Mix_VolumeMusic(0);
        this -> is_muted = false;
    }else{
        ui -> btn_muted -> setIcon(style() -> standardIcon(QStyle::SP_MediaVolume));
        Mix_VolumeMusic(ui -> spinBox -> value());
        this -> is_muted = true;
    }
}

void SDLAudioPlayer::on_slider_time_valueChanged(int value){
    if(!music){
        QMessageBox::information(this,"提示","请加载文件");
        return;
    }
    Mix_SetMusicPosition((double)value);
}

void SDLAudioPlayer::on_slider_volumn_valueChanged(int value){
    Mix_VolumeMusic(value);
    ui -> spinBox -> setValue(value);
}

void SDLAudioPlayer::on_spinBox_valueChanged(int arg1){
    ui -> slider_volumn -> setValue(arg1);
}

void SDLAudioPlayer::on_btn_clear_clicked(){
    ui -> tableWidget -> clear();
    this -> map.clear();
    this -> initTabel();
}

