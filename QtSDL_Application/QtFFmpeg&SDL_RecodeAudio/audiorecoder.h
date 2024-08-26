#ifndef AUDIORECODER_H
#define AUDIORECODER_H

#include <QThread>

#include <QMessageBox>
#include <QFile>
#include <QFileDialog>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

class AudioRecoder : public QThread
{
    Q_OBJECT
public:
    AudioRecoder(QObject *parent = nullptr);
    void stopRecod();
    void run()override;

signals:
    void sendRecodeTime(qint64 Mduration);

public:
    bool recording;
    QString fileName = "output.pcm";//指定默认保存文件名

};

#endif // AUDIORECODER_H
