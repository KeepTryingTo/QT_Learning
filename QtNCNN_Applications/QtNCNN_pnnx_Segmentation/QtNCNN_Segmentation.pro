QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    segmentationncnn.cpp

HEADERS += \
    segmentationncnn.h

FORMS += \
    segmentationncnn.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/MinGW_OpenCV/x64/mingw/lib/ -llibopencv_core455.dll \
                                                -llibopencv_highgui455.dll -llibopencv_imgcodecs455.dll -llibopencv_imgproc455.dll \
                                                -llibopencv_calib3d455.dll -llibopencv_ml455.dll -llibopencv_objdetect455.dll \
                                                -llibopencv_photo455.dll -llibopencv_dnn455.dll -llibopencv_features2d455.dll -llibopencv_video455.dll \
                                                -llibopencv_videoio455.dll
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/MinGW_OpenCV/x64/mingw/lib/ -llibopencv_core455.dll \
                                                -llibopencv_highgui455.dll -llibopencv_imgcodecs455.dll -llibopencv_imgproc455.dll \
                                                -llibopencv_calib3d455.dll -llibopencv_ml455.dll -llibopencv_objdetect455.dll \
                                                -llibopencv_photo455.dll -llibopencv_dnn455.dll -llibopencv_features2d455.dll -llibopencv_video455.dll \
                                                -llibopencv_videoio455.dll

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/MinGW_OpenCV/x64/mingw/bin/ -llibopencv_core455.dll \
                                                -llibopencv_highgui455.dll -llibopencv_imgcodecs455.dll -llibopencv_imgproc455.dll \
                                                -llibopencv_calib3d455.dll -llibopencv_ml455.dll -llibopencv_objdetect455.dll \
                                                -llibopencv_photo455.dll -llibopencv_dnn455.dll -llibopencv_features2d455.dll -llibopencv_video455.dll \
                                                -llibopencv_videoio455.dll
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/MinGW_OpenCV/x64/mingw/bin/ -llibopencv_core455.dll \
                                                -llibopencv_highgui455.dll -llibopencv_imgcodecs455.dll -llibopencv_imgproc455.dll \
                                                -llibopencv_calib3d455.dll -llibopencv_ml455.dll -llibopencv_objdetect455.dll \
                                                -llibopencv_photo455.dll -llibopencv_dnn455.dll -llibopencv_features2d455.dll -llibopencv_video455.dll \
                                                -llibopencv_videoio455.dll
INCLUDEPATH += $$PWD/../../../../OpenCV/MinGW_OpenCV/include
DEPENDPATH += $$PWD/../../../../OpenCV/MinGW_OpenCV/include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../conda3/Transfer_Learning/NCNN/installed_ncnn/lib/ -lncnn
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../conda3/Transfer_Learning/NCNN/installed_ncnn/lib/ -lncnn

INCLUDEPATH += $$PWD/../../../../../conda3/Transfer_Learning/NCNN/installed_ncnn/include
DEPENDPATH += $$PWD/../../../../../conda3/Transfer_Learning/NCNN/installed_ncnn/include

RESOURCES += \
    resources.qrc
