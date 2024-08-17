#include "opencvdetection.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    OpenCVDetection w;
    w.show();
    return a.exec();
}
