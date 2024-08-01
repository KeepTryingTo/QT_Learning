QT       += core gui network

# INCLUDEPATH +=$$PWD/libcurl/include
# LIBS += $$PWD/libcurl/libcurl.lib

# INCLUDEPATH += "C:/Program Files/CURL/include"
# LIBS += "C:/Program Files/CURL/lib/libcurl_imp.lib"
# LIBS += -L"C:/Program Files/CURL/bin/libcurl.dll"

INCLUDEPATH += D:/SoftwareFamily/QT/curl-8.9.1_1-win64-mingw/include
LIBS += -LD:/SoftwareFamily/QT/curl-8.9.1_1-win64-mingw/lib -lcurl
LIBS += -LD:/SoftwareFamily/QT/curl-8.9.1_1-win64-mingw/bin -lcurl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    ftpclient.cpp

HEADERS += \
    ftpclient.h

FORMS += \
    ftpclient.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
