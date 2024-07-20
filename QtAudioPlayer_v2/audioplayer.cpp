#include "audioplayer.h"
#include "ui_audioplayer.h"

#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QStyle>
#include <QFileDialog>

AudioPlayer::AudioPlayer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AudioPlayer)
{
    ui->setupUi(this);

    QFile file;
    file.setFileName(":/new/prefix1/resources/css/qthttpcss.css");
    file.open(QIODevice::ReadOnly);

    QString strCss = file.readAll();
    this -> setStyleSheet(strCss);

    this -> setWindowTitle("音频播放器");
    this -> setFixedSize(QSize(483,295));

    ui -> btn_player -> setIcon(style() -> standardIcon(QStyle::SP_MediaPlay));
    ui -> btn_pause -> setIcon(style() -> standardIcon(QStyle::SP_MediaPause));
    ui -> btn_stop -> setIcon(style() -> standardIcon(QStyle::SP_MediaStop));
    ui -> btn_seek_backward -> setIcon(style() -> standardIcon(QStyle::SP_MediaSeekBackward));
    ui -> btn_seek_feedward -> setIcon(style() -> standardIcon(QStyle::SP_MediaSeekForward));
    ui -> btn_muted -> setIcon(style() -> standardIcon(QStyle::SP_MediaVolumeMuted));

    this -> audioPlayer = new QMediaPlayer(this);
    this -> audioOutput = new QAudioOutput(this);
    //设置音频播放的音频输出到音频播放器中
    this -> audioPlayer -> setAudioOutput(audioOutput);
    //设置无限循环播放
    this -> audioPlayer -> setLoops(QMediaPlayer::Loops::Infinite);
    this -> audioOutput -> setVolume(ui -> progress_volumn -> value());

    connect(this -> audioPlayer,&QMediaPlayer::durationChanged,this,&AudioPlayer::durationChanged);
    connect(this -> audioPlayer,&QMediaPlayer::positionChanged,this,&AudioPlayer::positionChanged);

    ui -> progress_time -> setRange(0,this -> audioPlayer -> duration() / 1000);
}

AudioPlayer::~AudioPlayer(){
    delete ui;
}

void AudioPlayer::on_btn_open_file_clicked(){
    QString video_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("MP3 Files(*.mp3);;MP4 Files(*.mp4);;All Files(*.*)"));
    if(video_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    QFileInfo fileinformation(video_path);
    this -> fileName = fileinformation.fileName();
    ui -> audioName -> setText(this -> fileName);

    qDebug() <<"the video path is: "<< video_path;
    this -> audioPlayer -> setSource(QUrl::fromLocalFile(video_path));

    ui -> progress_volumn -> setMinimum(0);
    ui -> progress_volumn -> setMaximum(100);
    ui -> progress_volumn -> setValue(10);
}


void AudioPlayer::on_btn_close_clicked(){
    this -> close();
}


void AudioPlayer::on_btn_muted_clicked(){
    if(this -> is_muted == false){
        ui -> btn_muted -> setIcon(style() -> standardIcon(QStyle::SP_MediaVolumeMuted));
        this -> is_muted = true;
        this -> audioOutput -> setMuted(this -> is_muted);
    }else{
        ui -> btn_muted -> setIcon(style() -> standardIcon(QStyle::SP_MediaVolume));
        this -> is_muted = false;
        this -> audioOutput -> setMuted(this -> is_muted);
    }
}

void AudioPlayer::updateDuration(qint64 duration){
    QString timestr;
    if(duration || this -> Mduration){
        QTime CurrentTime((duration / 3600) % 60,(duration / 60) % 60,duration % 60,(duration * 1000) % 1000);
        QTime totalTime((Mduration / 3600) % 60,(Mduration / 60) % 60,Mduration % 60,(Mduration * 1000) % 1000);
        QString format = "mm:ss";
        if(this -> Mduration > 3600){
            format = "hh:mm:ss";
        }
        ui -> label_value_1 -> setText(CurrentTime.toString(format));
        ui -> label_value_2 -> setText(totalTime.toString(format));
    }
}


void AudioPlayer::on_btn_seek_backward_clicked(){
    qint64 initTime = ui -> progress_time -> value();
    ui -> progress_time -> setValue(initTime - 10);
    this -> audioPlayer -> setPosition(ui -> progress_time -> value() * 1000);
}

void AudioPlayer::on_btn_seek_feedward_clicked(){
    qint64 initTime = ui -> progress_time -> value();
    ui -> progress_time -> setValue(initTime + 10);
    this -> audioPlayer -> setPosition(ui -> progress_time -> value() * 1000);
}

void AudioPlayer::on_btn_stop_clicked(){
    this -> audioPlayer -> stop();
}


void AudioPlayer::on_btn_player_clicked(){
    this -> audioPlayer -> play();
}


void AudioPlayer::on_btn_pause_clicked(){
    this -> audioPlayer -> pause();
}

void AudioPlayer::on_progress_volumn_valueChanged(int value){
    this -> audioOutput -> setVolume(value);
}


void AudioPlayer::on_progress_time_valueChanged(int value){
    this -> audioPlayer -> setPosition(value * 1000);
}

void AudioPlayer::durationChanged(qint64 duration){
    this -> Mduration = duration / 1000;
    //获得一个音频总时长
    ui -> progress_time -> setMaximum(this -> Mduration);
}

void AudioPlayer::positionChanged(qint64 progress){
    if(!ui -> progress_time -> isSliderDown()){
        ui -> progress_time -> setValue(progress / 1000);
    }
    this -> updateDuration(progress / 1000);
}

