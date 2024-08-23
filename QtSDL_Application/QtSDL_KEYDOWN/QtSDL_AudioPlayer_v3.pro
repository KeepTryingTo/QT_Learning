QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    sdlaudioplayer.cpp

HEADERS += \
    sdlaudioplayer.h

FORMS += \
    sdlaudioplayer.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../SDL2-2.30.6/x86_64-w64-mingw32/lib/ -llibSDL2.dll
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../SDL2-2.30.6/x86_64-w64-mingw32/lib/ -llibSDL2.dll

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../SDL2-2.30.6/x86_64-w64-mingw32/bin/ -lSDL2.dll
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../SDL2-2.30.6/x86_64-w64-mingw32/bin/ -lSDL2.dll

INCLUDEPATH += $$PWD/../../../SDL2-2.30.6/x86_64-w64-mingw32/include
DEPENDPATH += $$PWD/../../../SDL2-2.30.6/x86_64-w64-mingw32/include
