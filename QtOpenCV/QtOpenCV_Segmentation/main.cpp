#include "opencvsegmantation.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    OpenCVSegmantation w;
    w.show();
    return a.exec();
}
