#include "sdlvideoplayer.h"
#include "ui_sdlvideoplayer.h"

SDLVideoPlayer::SDLVideoPlayer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SDLVideoPlayer)
{
    ui->setupUi(this);
}

SDLVideoPlayer::~SDLVideoPlayer()
{
    delete ui;
}


