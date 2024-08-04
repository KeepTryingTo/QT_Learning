#include "udpbroadcast.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    UdpBroadCast w;
    w.show();
    return a.exec();
}
