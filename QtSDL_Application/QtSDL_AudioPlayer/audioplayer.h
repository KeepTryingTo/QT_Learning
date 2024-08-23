#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QThread>
#include <QFile>
#include <QDebug>
#include <QTimer>

#include <SDL2/SDL.h>


struct userData {
    Uint8 * audio_buf;
    Uint32 audio_len;
    int curr_len;
};

class audioPlayer : public QThread
{
    Q_OBJECT
public:
    explicit audioPlayer(QObject *parent = nullptr);
    ~audioPlayer();
    void run() override;

public:
    bool isRunning;
    bool isPaused;
    QString fileName;
    SDL_AudioDeviceID device_id;
    userData * userdata;
    QTimer * progressTimer;

signals:
    void sendMduration(qint64 duration);
    void sendCurDuration(qint64 duration);
    void sendFinished();
};

#endif // AUDIOPLAYER_H
