QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    opencvdnn.cpp

HEADERS += \
    opencvdnn.h

FORMS += \
    opencvdnn.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/CMake_OpenCV/install/lib/ -lopencv_core455 -lopencv_highgui455 -lopencv_imgcodecs455 -lopencv_imgproc455 \
                                                -lopencv_calib3d455 -lopencv_ml455 -lopencv_objdetect455 -lopencv_photo455 -lopencv_dnn455 -lopencv_features2d455 -lopencv_video455
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/CMake_OpenCV/install/lib/ -lopencv_core455 -lopencv_highgui455 -lopencv_imgcodecs455 -lopencv_imgproc455 \
                                                -lopencv_calib3d455 -lopencv_ml455 -lopencv_objdetect455 -lopencv_photo455 -lopencv_dnn455 -lopencv_features2d455 -lopencv_video455

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/CMake_OpenCV/install/bin/ -lopencv_core455 -lopencv_highgui455 -lopencv_imgcodecs455 -lopencv_imgproc455 \
                                                -lopencv_calib3d455 -lopencv_ml455 -lopencv_objdetect455 -lopencv_photo455 -lopencv_dnn455 -lopencv_features2d455 -lopencv_video455
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/CMake_OpenCV/install/bin/ -lopencv_core455 -lopencv_highgui455 -lopencv_imgcodecs455 -lopencv_imgproc455 \
                                                -lopencv_calib3d455 -lopencv_ml455 -lopencv_objdetect455 -lopencv_photo455 -lopencv_dnn455 -lopencv_features2d455 -lopencv_video455

INCLUDEPATH += $$PWD/../../../../OpenCV/CMake_OpenCV/install/include
DEPENDPATH += $$PWD/../../../../OpenCV/CMake_OpenCV/install/include



# win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/software/opencv/build/x64/vc16/lib/ -lopencv_world4100d
# else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/software/opencv/build/x64/vc16/lib/ -lopencv_world4100d

# win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/software/opencv/build/x64/vc16/bin/ -lopencv_world4100d
# else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/software/opencv/build/x64/vc16/bin/ -lopencv_world4100d

# INCLUDEPATH += $$PWD/../../../../OpenCV/software/opencv/build/include
# DEPENDPATH += $$PWD/../../../../OpenCV/software/opencv/build/include
