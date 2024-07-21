#include "videorecorder.h"
#include "ui_videorecorder.h"

VideoRecorder::VideoRecorder(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::VideoRecorder)
{
    ui->setupUi(this);

    this -> Mcamera = new QCamera(QMediaDevices::defaultVideoInput());
    this -> audioInput = new QAudioInput(this);
    this -> videoRecorder = new QMediaRecorder(this);
    this -> mediaCaptureSession = new QMediaCaptureSession(this);

    this -> mediaCaptureSession -> setAudioInput(this -> audioInput);
    this -> mediaCaptureSession -> setRecorder(this -> videoRecorder);
    this -> mediaCaptureSession -> setVideoOutput(ui -> videoWidget);

    this -> videoRecorder -> setQuality(QMediaRecorder::HighQuality);
    this -> videoRecorder -> setOutputLocation(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/demo.mp4"));
    this -> videoRecorder -> setEncodingMode(QMediaRecorder::ConstantQualityEncoding);
    this -> videoRecorder -> setAudioChannelCount(this -> videoRecorder -> audioChannelCount());

    connect(this -> videoRecorder,&QMediaRecorder::durationChanged,this,&VideoRecorder::onDurationChanged);

    this -> cameraDevices = QMediaDevices::videoInputs();
    ui -> comboBox -> addItem("<None>");
    for(QCameraDevice & camera : this -> cameraDevices){
        ui -> comboBox -> addItem(camera.description());
    }

    connect(ui -> comboBox,&QComboBox::currentIndexChanged,this,&VideoRecorder::selectCamDevice);
}

VideoRecorder::~VideoRecorder(){
    delete ui;
}

void VideoRecorder::onDurationChanged(qint64 duration){
    ui -> label_time -> setText(QString("%1 s").arg(QString::number(duration / 1000)));
}

void VideoRecorder::selectCamDevice(int index)
{
    //遍历当前选择的设备是否等于可利用的设备，然后进行设备的配置
    for(const QCameraDevice &camera : this -> cameraDevices){
        if(camera.description() == ui -> comboBox->currentText()){
            this -> Mcamera -> setCameraDevice(camera);
            this -> mediaCaptureSession -> setCamera(this -> Mcamera);
            qDebug()<<"current select camera: "<<camera.description();

            //关于sink的作用，里面包含了关于帧的一些基本信息
            sink = this -> mediaCaptureSession -> videoSink();
            connect(sink,&QVideoSink::videoFrameChanged,this,&VideoRecorder::onFrameChanged);
            this -> Mcamera -> start();

            break;
        }
    }
}

void VideoRecorder::onFrameChanged(const QVideoFrame&frame){
    QSize size = frame.size();
    qDebug()<<"frame size: "<<size;
    ui -> label_resl -> setText(QString("%1 x %2").arg(QString::number(size.width()),QString::number(size.height())));
}

void VideoRecorder::on_btn_start_clicked(){
    this -> videoRecorder -> record();
}

void VideoRecorder::on_btn_pause_clicked(){
    this -> videoRecorder -> pause();
}

void VideoRecorder::on_btn_stop_clicked(){
    this -> videoRecorder -> stop();
    ui -> label_time -> setText("0");
}

void VideoRecorder::on_btn_close_clicked(){
    this -> close();
}



