QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

#method one
# INCLUDEPATH += D:\SoftwareFamily\OpenCV\CMake_OpenCV\install\include
# LIBS += D:\SoftwareFamily\OpenCV\CMake_OpenCV\install\bin\opencv_*.a
# LIBS += D:\SoftwareFamily\OpenCV\CMake_OpenCV\install\lib\opencv_*.a

#method two
# LIBS += D:\SoftwareFamily\OpenCV\CMake_OpenCV\install\lib\opencv_core455.lib
# LIBS += D:\SoftwareFamily\OpenCV\CMake_OpenCV\install\lib\opencv_highgui455.lib
# LIBS += D:\SoftwareFamily\OpenCV\CMake_OpenCV\install\lib\opencv_imgcodecs455.lib
# LIBS += D:\SoftwareFamily\OpenCV\CMake_OpenCV\install\lib\opencv_imgproc455.lib

# LIBS += D:\SoftwareFamily\OpenCV\CMake_OpenCV\install\bin\opencv_core455.dll
# LIBS += D:\SoftwareFamily\OpenCV\CMake_OpenCV\install\bin\opencv_highgui455.dll
# LIBS += D:\SoftwareFamily\OpenCV\CMake_OpenCV\install\bin\opencv_imgcodecs455.dll
# LIBS += D:\SoftwareFamily\OpenCV\CMake_OpenCV\install\bin\opencv_imgproc455.dll


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    vcopencv.cpp

HEADERS += \
    vcopencv.h

FORMS += \
    vcopencv.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


#method three
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/software/opencv/build/x64/vc14/lib/ -lopencv_world455d
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/software/opencv/build/x64/vc14/lib/ -lopencv_world455d

INCLUDEPATH += $$PWD/../../../../OpenCV/software/opencv/build/include
DEPENDPATH += $$PWD/../../../../OpenCV/software/opencv/build/include

# win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/CMake_OpenCV/install/lib/ -lopencv_core455 -lopencv_highgui455 -lopencv_imgcodecs455 -lopencv_imgproc455
# else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/CMake_OpenCV/install/lib/ -lopencv_core455 -lopencv_highgui455 -lopencv_imgcodecs455 -lopencv_imgproc455

# win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/CMake_OpenCV/install/bin/ -lopencv_core455 -lopencv_highgui455 -lopencv_imgcodecs455 -lopencv_imgproc455
# else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/CMake_OpenCV/install/bin/ -lopencv_core455 -lopencv_highgui455 -lopencv_imgcodecs455 -lopencv_imgproc455

# INCLUDEPATH += $$PWD/../../../../OpenCV/CMake_OpenCV/install/include
# DEPENDPATH += $$PWD/../../../../OpenCV/CMake_OpenCV/install/include
