#include "audioplayer.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AudioPlayer w;
    w.show();
    return a.exec();
}
