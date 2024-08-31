#include "videoplayer.h"
#include "ui_videoplayer.h"

videoPlayer::videoPlayer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::videoPlayer)
{
    ui->setupUi(this);
}

videoPlayer::~videoPlayer()
{
    delete ui;
}
