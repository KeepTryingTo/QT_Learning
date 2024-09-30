#include "classificationncnn_v2.h"


#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ClassificationNCNN_v2 w;
    w.show();
    return a.exec();
}
