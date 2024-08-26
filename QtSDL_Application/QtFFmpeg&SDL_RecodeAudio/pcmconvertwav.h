#ifndef PCMCONVERTWAV_H
#define PCMCONVERTWAV_H

#include <QThread>

#include <QString>
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

class pcmConvertWav : public QThread
{
    Q_OBJECT
public:
    explicit pcmConvertWav(QString inputPath,QString outPath,QObject *parent = nullptr);
    void pcmConvertWAV();

    void run()override;

public:
    QString inputPath;
    QString outputPath;
};

#endif // PCMCONVERTWAV_H
