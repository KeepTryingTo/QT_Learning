#include "udpgroupchat.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    UdpGroupChat w;
    w.show();
    return a.exec();
}
