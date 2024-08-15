#include "qtopencvclassify.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtOpenCVClassify w;
    w.show();
    return a.exec();
}
