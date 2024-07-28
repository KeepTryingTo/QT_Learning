#include "serversocket.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ServerSocket w;
    w.show();
    return a.exec();
}
