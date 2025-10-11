QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets sql websockets multimedia

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aigamecontroller.cpp \
    board.cpp \
    chatdialog.cpp \
    chessaicontroller.cpp \
    chessboard.cpp \
    chessboardwidget.cpp \
    chessgamecontroller.cpp \
    chessmainwindow.cpp \
    chesspiecewidget.cpp \
    connectiondialog.cpp \
    gameselectiondialog.cpp \
    ledindicator.cpp \
    logindialog.cpp \
    main.cpp \
    client.cpp \
    networkmanager.cpp \
    networkprotocol.cpp \
    onlineusersdialog.cpp \
    onlineusersmanager.cpp \
    player.cpp \
    soundmanager.cpp

HEADERS += \
    aigamecontroller.h \
    board.h \
    chatdialog.h \
    chessaicontroller.h \
    chessboard.h \
    chessboardwidget.h \
    chessgamecontroller.h \
    chessmainwindow.h \
    chesspiecewidget.h \
    client.h \
    common.h \
    connectiondialog.h \
    gameselectiondialog.h \
    ledindicator.h \
    logindialog.h \
    networkmanager.h \
    networkprotocol.h \
    onlineusersdialog.h \
    onlineusersmanager.h \
    piece.h \
    piecetype.h \
    player.h \
    soundmanager.h

FORMS += \
    client.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
