#include "camera.h"
#include "ui_camera.h"
#include <QCameraDevice>
#include <QMediaDevices>
#include <QComboBox>
#include <QVideoFrame>
#include <QSize>


Camera::Camera(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Camera)
{
    ui->setupUi(this);

    getCameras();

    this -> m_camera = new QCamera(QMediaDevices::defaultVideoInput());
    this -> mediaCaptureSession = new QMediaCaptureSession();

    connect(ui -> comboBox,&QComboBox::currentIndexChanged,this,&Camera::selectCam);
}

Camera::~Camera(){
    delete ui;
}

void Camera::getCameras(){
    //默认设备为None
    ui -> comboBox -> addItem("<None>");
    //首先遍历所有的可用设备
    this -> availCameraDevices = QMediaDevices::videoInputs();
    for(const QCameraDevice & camera : availCameraDevices){
        ui -> comboBox -> addItem(camera.description());
        // qDebug()<<"current available device: "<<camera.description();
    }
}
//退出当前程序
void Camera::on_btn_close_clicked(){
    this -> close();
}
//打开摄像头
void Camera::on_btn_start_clicked(){
    this -> m_camera -> start();
}

//关闭摄像头
void Camera::on_btn_stop_clicked(){
    if(this -> m_camera->isActive()){
        this -> m_camera->stop();
        disconnect(sink);
    }
}


void Camera::selectCam(int index){
    //遍历当前选择的设备是否等于可利用的设备，然后进行设备的配置
    for(const QCameraDevice &camera : availCameraDevices){
        if(camera.description() == ui -> comboBox->currentText()){
            this -> m_camera -> setCameraDevice(camera);
            this -> mediaCaptureSession -> setCamera(this -> m_camera);
            this -> mediaCaptureSession -> setVideoOutput(ui -> videowidget);
            qDebug()<<"current select camera: "<<camera.description();

            //关于sink的作用，里面包含了关于帧的一些基本信息
            sink = this -> mediaCaptureSession -> videoSink();
            connect(sink,&QVideoSink::videoFrameChanged,this,&Camera::onFrameChanged);

            break;
        }
    }
}

void Camera::onFrameChanged(const QVideoFrame&frame){
    QSize size = frame.size();
    qDebug()<<"frame size: "<<size;
    ui -> lab_resl -> setText(QString("%1 x %2").arg(QString::number(size.width()),QString::number(size.height())));
}

