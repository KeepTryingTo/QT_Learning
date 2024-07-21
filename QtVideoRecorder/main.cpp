#include "videorecorder.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    VideoRecorder w;
    w.show();
    return a.exec();
}
