#include "sdlaudioplayer.h"
#include "ui_sdlaudioplayer.h"

SDLAudioPlayer::SDLAudioPlayer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SDLAudioPlayer)
{
    ui->setupUi(this);
}

SDLAudioPlayer::~SDLAudioPlayer()
{
    delete ui;
}
