﻿#include "mediarecorder.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MediaRecorder w;
    w.show();
    return a.exec();
}
