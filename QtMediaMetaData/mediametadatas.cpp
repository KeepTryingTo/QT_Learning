#include "mediametadatas.h"
#include "ui_mediametadatas.h"

#include <QFile>
#include <QImage>
#include <QPixmap>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QAudioOutput>

MediaMetaDataS::MediaMetaDataS(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MediaMetaDataS)
{
    ui->setupUi(this);

    this -> player = new QMediaPlayer(this);

    //设置音频播放的音频输出到音频播放器中,一定要加入音频的输出才能改变状态得到LoadedMedia，从而输出音频中相关的标题，作者，专辑等信息...
    QAudioOutput*audioOutput = new QAudioOutput(this);
    this -> player -> setAudioOutput(audioOutput);

    ui -> plainTextEdit -> appendPlainText("以下包含了视频或者音频中标题，作者，专辑等信息......");

    connect(player,&QMediaPlayer::mediaStatusChanged,this,&MediaMetaDataS::OnMediaStatusChanged);
    // connect(player,SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),this,SLOT(OnMediaStatusChanged(QMediaPlayer::MediaStatus)));
}

MediaMetaDataS::~MediaMetaDataS(){
    delete ui;
}

void MediaMetaDataS::OnMediaStatusChanged(QMediaPlayer::MediaStatus status){
    //媒体加载完成，但尚未播放
    if(status == QMediaPlayer::LoadedMedia){
        QMediaMetaData meta = this -> player -> metaData();
        qDebug()<<"Title: "<<meta.stringValue(QMediaMetaData::Key::Title);
        qDebug()<<"Author: "<<meta.stringValue(QMediaMetaData::Key::Author);
        qDebug()<<"AlbumTitle: "<<meta.stringValue(QMediaMetaData::Key::AlbumTitle);
        qDebug()<<"Comment: "<<meta.stringValue(QMediaMetaData::Key::Comment);
        qDebug()<<"Description: "<<meta.stringValue(QMediaMetaData::Key::Description);
        qDebug()<<"Language: "<<meta.stringValue(QMediaMetaData::Key::Language);

        ui -> plainTextEdit -> appendPlainText("Title: " + meta.stringValue(QMediaMetaData::Key::Title));
        ui -> plainTextEdit -> appendPlainText("Author: " + meta.stringValue(QMediaMetaData::Key::Author));
        ui -> plainTextEdit -> appendPlainText("AlbumTitle: " + meta.stringValue(QMediaMetaData::Key::AlbumTitle));
        ui -> plainTextEdit -> appendPlainText("Comment: " + meta.stringValue(QMediaMetaData::Key::Comment));
        ui -> plainTextEdit -> appendPlainText("Description: " + meta.stringValue(QMediaMetaData::Key::Description));
        ui -> plainTextEdit -> appendPlainText("Language: " + meta.stringValue(QMediaMetaData::Key::Language));

        QImage image = meta.value(QMediaMetaData::Key::ThumbnailImage).value<QImage>();
        ui -> label_music_thumbnail -> setPixmap(QPixmap::fromImage(image));
    }
    if(status == QMediaPlayer::InvalidMedia){
        qDebug()<<"播放的音频无效";
        ui -> plainTextEdit -> appendPlainText("播放的音频无效");
    }
    // this -> player -> play();
}


void MediaMetaDataS::on_btn_open_file_clicked(){
    QString video_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",
                                                      tr("MP3 Files(*.mp3);;MP4 Files(*.mp4);;All Files(*.*)"));
    if(video_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    QFileInfo fileinformation(video_path);
    this -> fileName = fileinformation.fileName();
    ui -> audioName -> setText(this -> fileName);

    qDebug() <<"the video path is: "<< video_path;
    this -> player -> setSource(QUrl::fromLocalFile(video_path));
}


void MediaMetaDataS::on_btn_close_clicked(){
    this -> close();
}
/*
    UnknownMediaStatus：无法确定媒体的状态。
    NoMedia：没有当前的媒体。播放器处于 StoppedState。
    LoadingMedia：正在加载当前媒体。播放器可能处于任何状态。
    LoadedMedia：已加载当前媒体。播放器处于 StoppedState。
    StalledMedia：由于缓冲不足或其他临时中断，当前媒体的播放已停止。播放器处于播放状态或暂停状态。
    BufferedMedia：媒体正在缓冲数据，播放器处于播放状态或暂停状态（注意：在某些版本的 Qt 中，这个状态可能不存在，
                  而是用 StalledMedia 和其他状态来表示）。
    EndOfMedia：媒体结束，播放器处于 StoppedState。
    InvalidMedia：当前媒体无法播放。播放器处于 StoppedState。
*/
