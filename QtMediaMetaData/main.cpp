#include "mediametadatas.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MediaMetaDataS w;
    w.show();
    return a.exec();
}
