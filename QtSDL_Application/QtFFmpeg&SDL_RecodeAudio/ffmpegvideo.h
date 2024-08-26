#ifndef FFMPEGVIDEO_H
#define FFMPEGVIDEO_H

#include <QMainWindow>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMap>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include "audiorecoder.h"
#include "pcmconvertwav.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class FFmpegVideo;
}
QT_END_NAMESPACE

class FFmpegVideo : public QMainWindow
{
    Q_OBJECT

public:
    FFmpegVideo(QWidget *parent = nullptr);
    ~FFmpegVideo();

    void getAllDevices();
    void startRecord();
    void stopRecord();
    void initTabel();
    void addTableWidgetItem(QString filePath);
    void updateDuration(qint64 duration);
    void initMixer();

private slots:
    void on_pushButton_clicked();

    void on_btn_save_clicked();

    void on_btn_player_clicked();

    void on_btn_open_clicked();

    void on_btn_pcmTowav_clicked();

    void on_btn_pause_player_clicked();

signals:
    void sendStopSignal();
private:
    Ui::FFmpegVideo *ui;
    int rows = 0;
    qint64 Mduration;
    QString musicPath = "";
    bool isPaused = false;
    double currPosition = 0;
    QString globalPath;

    AudioRecoder * recoder;
    QMap<int,QString>map;
    Mix_Music * music;

    pcmConvertWav * pcm2wav;
};
#endif // FFMPEGVIDEO_H
