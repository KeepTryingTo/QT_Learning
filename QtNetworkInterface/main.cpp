#include "networkinterface.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NetworkInterface w;
    w.show();
    return a.exec();
}
