#ifndef SDLAUDIOPLAYER_H
#define SDLAUDIOPLAYER_H

#include <QMainWindow>

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSize>

#include <SDL2/SDL.h>

#include "audioplayer.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class SDLAudioPlayer;
}
QT_END_NAMESPACE

class SDLAudioPlayer : public QMainWindow
{
    Q_OBJECT

public:
    SDLAudioPlayer(QWidget *parent = nullptr);
    ~SDLAudioPlayer();

private slots:
    void on_btn_open_clicked();

    void on_btn_close_clicked();

    void on_btn_player_clicked();

    void updateDuration(qint64 duration);

private:
    Ui::SDLAudioPlayer *ui;
    qint64 Mduration;
    audioPlayer * audio_player;
};
#endif // SDLAUDIOPLAYER_H
