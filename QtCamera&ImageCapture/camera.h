#ifndef CAMERA_H
#define CAMERA_H

#include <QMainWindow>
#include <QtWidgets>
#include <QtMultimedia>
#include <QtMultimediaWidgets>
#include <QImageCapture>
#include <QCameraDevice>
#include <QMediaDevices>
#include <QList>
#include <QMediaCaptureSession>
#include <QVideoWidget>
#include <QVideoSink>
#include <QImage>

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

    void getCameras();
    void onFrameChanged(const QVideoFrame&frame);
    void displayViewfinder();
    
private slots:
    
    void on_btn_close_clicked();
    
    void on_btn_start_clicked();

    void on_btn_stop_clicked();

    void selectCam(int index);

    void imageSaved(int id, const QString &fileName);
    void displayImage();
    void displayCapturedImage();
    void displayCameraError();
    void processCapturedImage(int requestId, const QImage &img);
    void displayCaptureError(int id, const QImageCapture::Error error, const QString &errorString);

    void on_btn_capture_clicked();


private:
    Ui::Camera *ui;

    QCamera *m_camera;
    QVideoSink * sink;
    QScopedPointer<QImageCapture> imageCapture;
    QImage imageFromCamera;
    QImage image;

    QScopedPointer<QImageCapture> m_camera_capture;
    QList<QCameraDevice> availCameraDevices;
    //QMediaCaptureSession： https://blog.csdn.net/yegu001/article/details/135767425
    QMediaCaptureSession * mediaCaptureSession;

    int Counter = 0;
};
#endif // CAMERA_H
