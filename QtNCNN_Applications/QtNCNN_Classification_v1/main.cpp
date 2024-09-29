#include "classificationncnn.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ClassificationNCNN w;
    w.show();
    return a.exec();
}
