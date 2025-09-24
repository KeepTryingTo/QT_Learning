QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets websockets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    chatdialog.cpp \
    connectdialog.cpp \
    drawingtool.cpp \
    filemanager.cpp \
    helpmanager.cpp \
    ledindicator.cpp \
    main.cpp \
    client.cpp \
    networkprotocol.cpp \
    roomdialog.cpp \
    websocketmanager.cpp

HEADERS += \
    chatdialog.h \
    client.h \
    connectdialog.h \
    drawingtool.h \
    filemanager.h \
    helpmanager.h \
    ledindicator.h \
    networkprotocol.h \
    roomdialog.h \
    websocketmanager.h

FORMS += \
    client.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
