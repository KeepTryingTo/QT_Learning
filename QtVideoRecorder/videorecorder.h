#ifndef VIDEORECORDER_H
#define VIDEORECORDER_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioInput>
#include <QAudioOutput>
#include <QVideoFrame>
#include <QVideoSink>
#include <QMediaRecorder>
#include <QVideoWidget>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QCamera>
#include <QCameraDevice>

#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class VideoRecorder;
}
QT_END_NAMESPACE

class VideoRecorder : public QMainWindow
{
    Q_OBJECT

public:
    VideoRecorder(QWidget *parent = nullptr);
    ~VideoRecorder();

private slots:
    void onDurationChanged(qint64 duration);
    void selectCamDevice(int index);
    void onFrameChanged(const QVideoFrame&frame);

    void on_btn_start_clicked();

    void on_btn_pause_clicked();

    void on_btn_stop_clicked();

    void on_btn_close_clicked();

private:
    Ui::VideoRecorder *ui;
    QString fileName;
    QList<QCameraDevice> cameraDevices;

    QCamera * Mcamera;
    QVideoSink * sink;
    QMediaDevices * mediaDevices;
    QAudioInput * audioInput;
    QMediaPlayer * audioPlayer;
    QMediaRecorder * videoRecorder;
    QMediaCaptureSession * mediaCaptureSession;
};
#endif // VIDEORECORDER_H
