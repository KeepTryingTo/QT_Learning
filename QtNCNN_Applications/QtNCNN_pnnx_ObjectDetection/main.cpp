#include "objectdetectionncnn.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ObjectDetectionNCNN w;
    w.show();
    return a.exec();
}
