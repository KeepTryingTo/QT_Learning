#include "mediarecorder.h"
#include "ui_mediarecorder.h"

MediaRecorder::MediaRecorder(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MediaRecorder)
{
    ui->setupUi(this);

    this -> mediaRecorder = new QMediaRecorder(this);
    this -> mediaCaptureSession = new QMediaCaptureSession(this);
    this -> audioInput = new QAudioInput(this);

    this -> mediaCaptureSession -> setAudioInput(this -> audioInput);
    this -> mediaCaptureSession -> setRecorder(this -> mediaRecorder);

    this -> mediaRecorder -> setQuality(QMediaRecorder::HighQuality);
    this -> mediaRecorder -> setOutputLocation(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/demo.mp3"));
    this -> mediaRecorder -> setEncodingMode(QMediaRecorder::ConstantQualityEncoding);
    this -> mediaRecorder -> setAudioChannelCount(this -> mediaRecorder -> audioChannelCount());

    connect(this -> mediaRecorder,&QMediaRecorder::durationChanged,this,&MediaRecorder::onDurationChanged);

    ui -> plainTextEdit -> appendPlainText("Quality: " + QString::number(this -> mediaRecorder -> quality()));
    ui -> plainTextEdit -> appendPlainText("Save path: " + (this -> mediaRecorder -> outputLocation()).toString());
    ui -> plainTextEdit -> appendPlainText("Encode Mode: " + QString::number(this -> mediaRecorder -> encodingMode()));
    ui -> plainTextEdit -> appendPlainText("Channel Count: " + QString::number(this -> mediaRecorder -> audioChannelCount()));
}

MediaRecorder::~MediaRecorder(){
    delete ui;
}

void MediaRecorder::onDurationChanged(qint64 duration){
    ui -> label_time -> setText(QString("%1 s").arg(QString::number(duration / 1000)));
}

void MediaRecorder::on_btn_start_clicked(){
    this -> mediaRecorder -> record();
}

void MediaRecorder::on_btn_pause_clicked(){
    this -> mediaRecorder -> pause();
}

void MediaRecorder::on_btn_stop_clicked(){
    this -> mediaRecorder -> stop();
    ui -> label_time -> setText("0");
}

void MediaRecorder::on_btn_close_clicked(){
    this -> close();
}

