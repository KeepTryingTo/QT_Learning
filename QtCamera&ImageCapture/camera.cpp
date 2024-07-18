#include "camera.h"
#include "ui_camera.h"
#include <QCameraDevice>
#include <QMediaDevices>
#include <QComboBox>
#include <QVideoFrame>
#include <QSize>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QMessageBox>


Camera::Camera(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Camera)
{
    ui->setupUi(this);

    getCameras();

    this -> m_camera = new QCamera(QMediaDevices::defaultVideoInput());
    this -> mediaCaptureSession = new QMediaCaptureSession();

    connect(ui -> comboBox,&QComboBox::currentIndexChanged,this,&Camera::selectCam);

    //拍照功能
    this -> imageCapture.reset(new QImageCapture);
    this -> mediaCaptureSession -> setImageCapture(this -> imageCapture.get());
    connect(this -> imageCapture.get(), &QImageCapture::imageCaptured, this,&Camera::processCapturedImage);
    connect(this -> imageCapture.get(),&QImageCapture::imageSaved,this,&Camera::imageSaved);
    connect(this -> imageCapture.get(), &QImageCapture::errorOccurred, this,&Camera::displayCaptureError);
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

void Camera::processCapturedImage(int requestId, const QImage& img){
    Q_UNUSED(requestId);
    QImage scaledImage = img.scaled(ui->videowidget->size(),
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);
    //显示当前图像，4秒之后不再显示，默认保存到系统相册路径下
    ui->viewfinder->setPixmap(QPixmap::fromImage(scaledImage));
    QTimer::singleShot(4000,this,&Camera::displayImage);
    displayCapturedImage();
}
//显示拍照的图像在当前第0个画布
void Camera::displayImage(){
    ui -> stackedWidget -> setCurrentIndex(0);
}

void Camera::displayCapturedImage(){
    ui -> stackedWidget -> setCurrentIndex(1);
}

void Camera::displayCaptureError(int id, const QImageCapture::Error error, const QString &errorString){
    Q_UNUSED(id);
    Q_UNUSED(error);
    QMessageBox::warning(this, tr("Image Capture Error"), errorString);
}

void Camera::displayCameraError(){
    if (m_camera->error() != QCamera::NoError)
        QMessageBox::warning(this, tr("Camera Error"), m_camera->errorString());
}

void Camera::on_btn_capture_clicked(){
    //捕获图像，同时保存到系统默认相册路径下
    this -> imageCapture -> captureToFile();
}

void Camera::imageSaved(int id, const QString &fileName)
{
    Q_UNUSED(id);
    //默认保存到系统相册文件下
    QFileInfo fileInfo(tr("Captured \"%1\"").arg(QDir::toNativeSeparators(fileName)));
    ui->lab_path->setText(fileInfo.fileName());
}

