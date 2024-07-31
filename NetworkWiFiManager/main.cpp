#include "networkwifimanager.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NetworkWifiManager w;
    w.show();
    return a.exec();
}
