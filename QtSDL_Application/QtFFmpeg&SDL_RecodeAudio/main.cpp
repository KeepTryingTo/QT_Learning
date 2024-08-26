#include "ffmpegvideo.h"

#include <QApplication>

#undef main

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FFmpegVideo w;
    w.show();
    return a.exec();
}
