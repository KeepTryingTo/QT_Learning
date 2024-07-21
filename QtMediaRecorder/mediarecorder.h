#ifndef MEDIARECORDER_H
#define MEDIARECORDER_H

#include <QMainWindow>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QString>
#include <QMessageBox>
#include <QList>
#include <QtMultimedia>
#include <QMediaRecorder>
#include <QAudioOutput>
#include <QAudioInput>
#include <QtCore>
#include <QtWidgets>
#include <QtGui>
#include <QMediaCaptureSession>

QT_BEGIN_NAMESPACE
namespace Ui {
class MediaRecorder;
}
QT_END_NAMESPACE

class MediaRecorder : public QMainWindow
{
    Q_OBJECT

public:
    MediaRecorder(QWidget *parent = nullptr);
    ~MediaRecorder();

private slots:
    void onDurationChanged(qint64 duration);

    void on_btn_start_clicked();

    void on_btn_pause_clicked();

    void on_btn_stop_clicked();

    void on_btn_close_clicked();

private:
    Ui::MediaRecorder *ui;

    QMediaRecorder * mediaRecorder;
    QMediaCaptureSession * mediaCaptureSession;
    QAudioInput * audioInput;
};
#endif // MEDIARECORDER_H
