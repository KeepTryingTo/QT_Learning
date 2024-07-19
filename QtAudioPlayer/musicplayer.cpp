#include "musicplayer.h"
#include "ui_musicplayer.h"

#include <QTime>
#include <QAudioOutput>

MusicPlayer::MusicPlayer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MusicPlayer)
{
    ui->setupUi(this);

    QFile file;
    file.setFileName(":/new/prefix1/resources/css/qthttpcss.css");
    file.open(QIODevice::ReadOnly);

    QString strCss = file.readAll();
    this -> setStyleSheet(strCss);

    this -> setWindowTitle("音频播放器");
    this -> setFixedSize(QSize(441,188));

    ui -> label_volume -> setText(QString("10/100"));
    this -> audioPlayer = new QMediaPlayer(this);
    this -> audioOutput = new QAudioOutput(this);

    this -> time = new QTime;

    //设置音频播放的音频输出到音频播放器中
    this -> audioPlayer -> setAudioOutput(audioOutput);
    //设置无限循环播放
    this -> audioPlayer -> setLoops(QMediaPlayer::Loops::Infinite);

    connect(this -> audioPlayer,SIGNAL(playbackStateChanged(QMediaPlayer::PlaybackState)),
            this,SLOT(onStateChanged(QMediaPlayer::PlaybackState)));
}

MusicPlayer::~MusicPlayer()
{
    delete ui;
}

void MusicPlayer::on_btn_open_file_clicked()
{
    QString video_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("MP3 Files(*.mp3);;MP4 Files(*.mp4);;All Files(*.*)"));
    if(video_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    QFileInfo fileinformation(video_path);
    this -> fileName = fileinformation.fileName();

    qDebug() <<"the video path is: "<< video_path;
    this -> audioPlayer -> setSource(QUrl::fromLocalFile(video_path));
}

void MusicPlayer::onStateChanged(QMediaPlayer::PlaybackState state){
    switch(state){
    case QMediaPlayer::PlayingState:
        ui -> lab_music_name -> setText(this -> fileName + QString(": 播放状态"));
        break;
    case QMediaPlayer::PausedState:
        ui -> lab_music_name -> setText(this -> fileName + QString(": 暂停播放状态"));
        break;
    case QMediaPlayer::StoppedState:
        ui -> lab_music_name -> setText(this -> fileName + QString(": 停止播放"));
        break;
    default:
        break;
    }
}

void MusicPlayer::on_btn_start_clicked(){
    this -> audioPlayer -> play();
}

void MusicPlayer::on_btn_pause_clicked(){
    this -> audioPlayer -> pause();
}

void MusicPlayer::on_btn_close_clicked(){
    this -> close();
}

void MusicPlayer::on_slider_volume_valueChanged(int value){
    this -> audioOutput -> setVolume(value);
    QString volume = QString("%1 / %2").arg(QString::number(value),QString::number(100));
    ui -> label_volume -> setText(volume);
}

