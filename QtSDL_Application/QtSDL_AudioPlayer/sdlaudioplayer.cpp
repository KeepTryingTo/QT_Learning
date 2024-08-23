#include "sdlaudioplayer.h"
#include "ui_sdlaudioplayer.h"

SDLAudioPlayer::SDLAudioPlayer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SDLAudioPlayer)
{
    ui->setupUi(this);

    this -> audio_player = new audioPlayer;

    SDL_version  ver;
    SDL_VERSION(&ver);
    ui -> plainTextEdit -> appendPlainText(
        "SDL version: " + QString::number(ver.major) + " " + QString::number(ver.minor) + " " + QString::number(ver.patch)
    );

    connect(this,&QMainWindow::destroyed,this -> audio_player,[=](){
        this -> audio_player -> exit();
        this -> audio_player -> wait();
        this -> audio_player -> deleteLater();
    });
}

SDLAudioPlayer::~SDLAudioPlayer()
{
    delete ui;
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

        QFileInfo fileinformation(this -> audio_player -> fileName);
        ui -> plainTextEdit -> appendPlainText(fileinformation.fileName() + "总时长: " + CurrentTime.toString(format));
    }
}

void SDLAudioPlayer::on_btn_open_clicked()
{
    QString wav_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(wav_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    QFileInfo fileinformation(wav_path);

    ui -> plainTextEdit -> appendPlainText("播放音频: " + fileinformation.fileName());
    this -> audio_player -> fileName = wav_path;
}

void SDLAudioPlayer::on_btn_player_clicked(){
    if(this -> audio_player -> fileName.isEmpty()){
        QMessageBox::information(this,"提示","请选择WAV音频文件");
        return;
    }

    this -> audio_player -> start();
    //得到音频文件的总时长
    connect(this -> audio_player,&audioPlayer::sendMduration,this,[=](qint64 mduration){
        this -> Mduration = mduration;
        this -> updateDuration(mduration);
    });

    // QFileInfo fileinformation(this -> audio_player -> fileName);
    // connect(this -> audio_player,&audioPlayer::sendFinished,this,[=](){
    //     ui -> plainTextEdit -> appendPlainText(fileinformation.fileName() + "播放完毕");
    // });
}

void SDLAudioPlayer::on_btn_close_clicked(){
    this -> audio_player -> isRunning = false;
    this -> close();
}


