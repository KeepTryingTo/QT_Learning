#include "sdlffmpegvideoplayer.h"
#include "ui_sdlffmpegvideoplayer.h"

SDLFFmpegVideoPlayer::SDLFFmpegVideoPlayer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SDLFFmpegVideoPlayer)
{
    ui->setupUi(this);
}

SDLFFmpegVideoPlayer::~SDLFFmpegVideoPlayer()
{
    delete ui;
}
