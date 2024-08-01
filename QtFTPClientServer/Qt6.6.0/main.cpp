#include "ftpclient.h"

#include <QApplication>
#include <QFile>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // QCoreApplication::addLibraryPath("D:\\SoftwareFamily\\QT\\curl-8.9.1_1-win64-mingw\\bin");

    FTPClient w;
    w.show();

    return a.exec();
}

