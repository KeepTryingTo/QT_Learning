#ifndef CAMERA_H
#define CAMERA_H

#include <QMainWindow>

#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QIODevice>
#include <QMessageBox>

#include <QMediaMetaData>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QCamera>
#include <QAudioInput>
#include <QVideoFrame>
#include <QAudioOutput>
#include <QAudioSink>
#include <QVideoSink>
#include <QVideoWidget>
#include <QImageCapture>
#include <QMediaCaptureSession>
#include <QImage>
#include <QCameraDevice>
#include <QMediaRecorder>

#include "metadatadialog.h"
#include "imagesettings.h"
#include "videosettings.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Camera;
}
QT_END_NAMESPACE

class Camera : public QMainWindow
{
    Q_OBJECT

public:
    Camera(QWidget *parent = nullptr);
    ~Camera();

    void setCamera();
    void showMetaDataDialog();
    void saveMetaData();

private slots:
    void selectCam(int index);
    void onFrameChanged(const QVideoFrame&frame);
    void processCapturedImage(int requestId, const QImage& img);
    void displayImage();
    void displayCapturedImage();
    void displayCaptureError(int id, const QImageCapture::Error error, const QString &errorString);
    void displayCameraError();
    void imageSaved(int id, const QString &fileName);

    void selectPage(int index);

    void onDurationChanged(qint64 duration);

    void configureVideoSettings();
    void configureImageSettings();

    void on_pushButton_5_clicked();

    void on_btn_capture_clicked();

    void on_btn_camera_start_clicked();

    void on_btn_camera_stop_clicked();

    void on_btn_record_clicked();

    void on_btn_record_pause_clicked();

    void on_btn_record_stop_clicked();

    void on_btn_record_metaData_clicked();

    void on_btn_record_muted_clicked();

    void on_btn_camera_setting_clicked();

    void on_btn_video_setting_clicked();

private:
    Ui::Camera *ui;

    bool muted = false;
    QString fileName;
    bool isCapture = true;
    bool isRecordVideo = false;
    QList<QCameraDevice> cameraDevices;

    QCamera * Mcamera;
    QVideoSink * sink;
    QImage * image;
    QAudioInput * audioInput;
    QAudioOutput * audioOutput;
    QMediaPlayer * mediaPlayer;
    QScopedPointer<QMediaRecorder> mediaRecorder;
    QScopedPointer<QImageCapture>imageCapture;
    QMediaCaptureSession * mediaCaptureSession;

    MetaDataDialog *m_metaDataDialog = nullptr;

};
#endif // CAMERA_H
