#include "vcopencv.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    VCOpenCV w;
    w.show();
    return a.exec();
}
