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
    
private slots:
    
    void on_btn_close_clicked();
    
    void on_btn_start_clicked();

    void on_btn_stop_clicked();

    void selectCam(int index);

private:
    Ui::Camera *ui;

    QCamera *m_camera;
    QVideoSink * sink;

    QScopedPointer<QImageCapture> m_camera_capture;
    QList<QCameraDevice> availCameraDevices;
    //QMediaCaptureSession： https://blog.csdn.net/yegu001/article/details/135767425
    QMediaCaptureSession * mediaCaptureSession;

    int Counter = 0;
    QImage imageFromCamera;
};
#endif // CAMERA_H
