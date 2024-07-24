#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QMainWindow>

#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QSize>

#include <QMediaPlayer>
#include <QVideoWidget>
#include <QVideoSink>
#include <QMediaCaptureSession>
#include <QAudioInput>
#include <QAudioOutput>

#include "metadatadialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class VideoPlayer;
}
QT_END_NAMESPACE

class VideoPlayer : public QMainWindow
{
    Q_OBJECT

public:
    VideoPlayer(QWidget *parent = nullptr);
    ~VideoPlayer();

    void updateDuration(qint64 duration);
    void showMetaDataDialog();

private slots:
    void on_btn_close_clicked();

    void on_btn_open_file_clicked();

    void on_btn_seek_backward_clicked();

    void on_btn_stop_clicked();

    void on_btn_start_clicked();

    void on_btn_pause_clicked();

    void on_btn_seek_feedward_clicked();

    void on_btn_muted_clicked();

    void on_btn_cycle_start_clicked();

    void on_slider_time_valueChanged(int value);

    void on_slider_volumn_valueChanged(int value);

    void on_btn_metaData_clicked();

    void on_btn_clear_clicked();

    void durationChanged(qint64 duration);
    void positionChanged(qint64 progress);
    void selectSpeed(int index);

private:
    Ui::VideoPlayer *ui;

    QString fileName;
    qint64 Mduration;
    bool is_muted = false;
    double speed = 1.0;

    QAudioOutput * audioOutput;
    QMediaPlayer * mediaPlayer;
    QMediaCaptureSession * mediaCaptureSession;
    QVideoSink * sink;
    MetaDataDialog * m_metaDataDialog = nullptr;
};
#endif // VIDEOPLAYER_H
