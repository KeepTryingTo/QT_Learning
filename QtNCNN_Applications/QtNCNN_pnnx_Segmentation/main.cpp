#include "segmentationncnn.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SegmentationNCNN w;
    w.show();
    return a.exec();
}
