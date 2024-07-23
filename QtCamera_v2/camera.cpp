#include "camera.h"
#include "ui_camera.h"

#include <QSize>
#include <QStyle>
#include <QTimer>
#include <QTime>

Camera::Camera(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Camera)
{
    ui->setupUi(this);

    QFile file;
    file.setFileName(":/new/prefix1/resources/css/qthttpcss.css");
    file.open(QIODevice::ReadOnly);

    QString strCss = file.readAll();
    this -> setStyleSheet(strCss);
    this -> setFixedSize(QSize(601,298));

    this -> mediaPlayer = new QMediaPlayer(this);
    this -> Mcamera = new QCamera(QMediaDevices::defaultVideoInput());
    this -> cameraDevices = QMediaDevices::videoInputs();
    ui -> comboBox -> addItem("<None>");
    for(QCameraDevice & camera : this -> cameraDevices){
        ui -> comboBox -> addItem(camera.description());
    }

    this -> mediaRecorder.reset(new QMediaRecorder);
    this -> audioInput = new QAudioInput;
    this -> audioOutput = new QAudioOutput;
    this -> mediaCaptureSession = new QMediaCaptureSession;

    connect(ui -> comboBox,&QComboBox::currentIndexChanged,this,&Camera::selectCam);
    //当 QStackedWidget 当前显示的页面（小部件）发生变化时，currentChanged 信号会被发射
    connect(ui -> stackedWidget,&QStackedWidget::currentChanged,this,&Camera::selectPage);

    this -> setCamera();
}

Camera::~Camera(){
    delete ui;
}

void Camera::setCamera()
{
    //拍照功能
    this -> imageCapture.reset(new QImageCapture);
    this -> mediaCaptureSession -> setImageCapture(this -> imageCapture.get());
    connect(this -> imageCapture.get(), &QImageCapture::imageCaptured, this,&Camera::processCapturedImage);
    connect(this -> imageCapture.get(),&QImageCapture::imageSaved,this,&Camera::imageSaved);
    connect(this -> imageCapture.get(), &QImageCapture::errorOccurred, this,&Camera::displayCaptureError);

    //录像
    this -> mediaCaptureSession -> setAudioInput(this -> audioInput);
    this -> mediaCaptureSession -> setRecorder(this -> mediaRecorder.data());
    this -> mediaCaptureSession -> setVideoOutput(ui -> videoWidget);

    this -> mediaRecorder -> setQuality(QMediaRecorder::HighQuality);
    this -> mediaRecorder -> setOutputLocation(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/demo.mp4"));
    this -> mediaRecorder -> setEncodingMode(QMediaRecorder::ConstantQualityEncoding);
    this -> mediaRecorder -> setAudioChannelCount(this -> mediaRecorder -> audioChannelCount());

    connect(this -> mediaRecorder.data(),&QMediaRecorder::durationChanged,this,&Camera::onDurationChanged);

}

//选择stackwidget页面
void Camera::selectPage(int index){
    qDebug()<<"current page index: "<<index;
    if(index == 0){
        this -> isCapture = true;
        this -> isRecordVideo = false;
    }else if(index == 1){
        if(this -> Mcamera->isActive()){
            this -> Mcamera->stop();
            disconnect(sink);
        }
        this -> isCapture = false;
        this -> isRecordVideo = true;
    }
}

void Camera::selectCam(int index){
    //遍历当前选择的设备是否等于可利用的设备，然后进行设备的配置
    for(const QCameraDevice &camera : this -> cameraDevices){
        if(camera.description() == ui -> comboBox->currentText()){
            this -> Mcamera -> setCameraDevice(camera);
            this -> mediaCaptureSession -> setCamera(this -> Mcamera);
            this -> mediaCaptureSession -> setVideoOutput(ui -> videoWidget);
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
    // qDebug()<<"frame size: "<<size;
    ui -> lab_resl -> setText(QString("%1 x %2").arg(QString::number(size.width()),QString::number(size.height())));
}

void Camera::processCapturedImage(int requestId, const QImage& img){
    Q_UNUSED(requestId);
    QImage scaledImage = img.scaled(ui->videoWidget->size(),
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);
    //显示当前图像，4秒之后不再显示，默认保存到系统相册路径下
    ui->labelImage->setPixmap(QPixmap::fromImage(scaledImage));
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
    if (this -> Mcamera->error() != QCamera::NoError)
        QMessageBox::warning(this, tr("Camera Error"), this -> Mcamera->errorString());
}

void Camera::imageSaved(int id, const QString &fileName)
{
    Q_UNUSED(id);
    //默认保存到系统相册文件下
    QFileInfo fileInfo(tr("Captured \"%1\"").arg(QDir::toNativeSeparators(fileName)));
    ui->label_path->setText(fileInfo.fileName());
}

//关闭程序
void Camera::on_pushButton_5_clicked(){
    this -> close();
}

//拍照
void Camera::on_btn_capture_clicked(){
    //检查设备是否准备好进行捕获。当此方法返回True时，表示可以进行拍照。
    if(this -> imageCapture -> isReadyForCapture()){
        this -> imageCapture -> captureToFile();
    }
    this -> Mcamera -> start();
}

//打开摄像头
void Camera::on_btn_camera_start_clicked(){
    this -> Mcamera -> start();
}

//关闭摄像头
void Camera::on_btn_camera_stop_clicked(){
    if(this -> Mcamera->isActive()){
        this -> Mcamera->stop();
        disconnect(sink);
    }
}

void Camera::onDurationChanged(qint64 duration){
    ui -> label_recorde_time -> setText(QString("%1 s").arg(QString::number(duration / 1000)));
}

void Camera::on_btn_record_clicked(){
    this -> Mcamera -> start();
    this -> mediaRecorder -> record();
}

void Camera::on_btn_record_pause_clicked(){
    this -> mediaRecorder -> pause();
}

void Camera::on_btn_record_stop_clicked(){

    this -> mediaRecorder -> stop();
    this -> Mcamera -> stop();
    ui -> label_recorde_time -> setText("0");
}

void Camera::showMetaDataDialog(){
    if (!m_metaDataDialog)
        m_metaDataDialog = new MetaDataDialog(this);
    //当这个Qt::WA_DeleteOnClose属性被设置到一个窗口或对话框上时，如果该窗口或对话框被关闭（无论是通过用户操作还是通过编程方式），
    //Qt将自动删除该窗口或对话框的实例。
    m_metaDataDialog->setAttribute(Qt::WA_DeleteOnClose, false);
    //当弹出的对话框按下OK按钮之后保存元数据,当调用这个Exec函数时，它会以模态方式显示对话框，并阻塞其他窗口的用户输入，直到当前对话框被关闭。
    if (m_metaDataDialog->exec() == QDialog::Accepted)
        saveMetaData();
}

void Camera::saveMetaData(){
    QMediaMetaData data;
    for (int i = 0; i < QMediaMetaData::NumMetaData; i++) {
        QString val = m_metaDataDialog->m_metaDataFields[i]->text();
        if (!val.isEmpty()) {
            auto key = static_cast<QMediaMetaData::Key>(i);
            if (i == QMediaMetaData::CoverArtImage) {
                QImage coverArt(val);
                data.insert(key, coverArt);
            }
            else if (i == QMediaMetaData::ThumbnailImage) {
                QImage thumbnail(val);
                data.insert(key, thumbnail);
            }
            else if (i == QMediaMetaData::Date) {
                QDateTime date = QDateTime::fromString(val);
                data.insert(key, date);
            }
            else {
                data.insert(key, val);
            }
        }
    }
    this -> mediaRecorder->setMetaData(data);
}

void Camera::on_btn_record_metaData_clicked(){
    this -> showMetaDataDialog();
}

void Camera::on_btn_record_muted_clicked(){
    this -> mediaCaptureSession -> audioInput() -> setMuted(!(this -> muted));
    this -> muted = true;
}

void Camera::configureVideoSettings(){
    VideoSettings settingsDialog(mediaRecorder.data());
    settingsDialog.setWindowFlags(settingsDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    if (settingsDialog.exec() == QDialog::Accepted){
        settingsDialog.applySettings();
        // qDebug()<<"configureVideoSettings";
    }
}

void Camera::configureImageSettings(){
    ImageSettings settingsDialogs(imageCapture.get());
    // qDebug()<<"configureImageSettings";
    settingsDialogs.setWindowFlags(settingsDialogs.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    if (settingsDialogs.exec() == QDialog::Accepted) {
        settingsDialogs.applyImageSettings();
    }
}

void Camera::on_btn_camera_setting_clicked(){
    configureImageSettings();
}

void Camera::on_btn_video_setting_clicked(){
    configureVideoSettings();
}
