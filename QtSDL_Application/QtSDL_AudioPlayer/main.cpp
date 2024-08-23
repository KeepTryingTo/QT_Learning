#include "sdlaudioplayer.h"

#include <QApplication>

#undef main

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SDLAudioPlayer w;
    w.show();
    return a.exec();
}
