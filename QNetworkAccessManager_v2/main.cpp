#include "networkaccessmanagers.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NetworkAccessManagers w;
    w.show();
    return a.exec();
}
