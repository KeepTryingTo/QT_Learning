QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    bullet.cpp \
    collisiondetector.cpp \
    enemy.cpp \
    enemycontroller.cpp \
    gamescene.cpp \
    main.cpp \
    flightshooting.cpp \
    playerspaceship.cpp \
    scoremanager.cpp \
    startmenu.cpp

HEADERS += \
    bullet.h \
    collisiondetector.h \
    enemy.h \
    enemycontroller.h \
    flightshooting.h \
    gamescene.h \
    playerspaceship.h \
    scoremanager.h \
    startmenu.h

FORMS += \
    flightshooting.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
