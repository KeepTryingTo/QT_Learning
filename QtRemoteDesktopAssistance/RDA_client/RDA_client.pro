QT       += core gui openglwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets websockets openglwidgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ledindicator.cpp \
    main.cpp \
    client.cpp \
    remotescreenwidget.cpp \
    sendfile.cpp

HEADERS += \
    client.h \
    ledindicator.h \
    remotescreenwidget.h \
    sendfile.h

FORMS += \
    client.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

win32 {
    LIBS += -lopengl32
    LIBS += -lglu32
}

LIBS += -lopengl32
