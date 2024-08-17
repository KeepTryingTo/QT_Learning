QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    opencvdetection.cpp

HEADERS += \
    opencvdetection.h

FORMS += \
    opencvdetection.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/build_opencv470/install/x64/mingw/lib/ -llibopencv_core470 \
#                                                 -llibopencv_highgui470 -llibopencv_imgcodecs470 -lopencv_imgproc470 \
#                                                 -llibopencv_calib3d470 -llibopencv_ml470 -llibopencv_objdetect470 -llibopencv_photo470 -llibopencv_dnn470 \
#                                                 -llibopencv_features2d470 -llibopencv_video470 -llibopencv_videoio470
# else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/build_opencv470/install/x64/mingw/lib/ -llibopencv_core470 \
#                                                 -llibopencv_highgui470 -llibopencv_imgcodecs470 -lopencv_imgproc470 \
#                                                 -llibopencv_calib3d470 -llibopencv_ml470 -llibopencv_objdetect470 -llibopencv_photo470 -llibopencv_dnn470 \
#                                                 -llibopencv_features2d470 -llibopencv_video470 -llibopencv_videoio470

# win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/build_opencv470/install/x64/mingw/bin/ -llibopencv_core470 \
#                                                 -llibopencv_highgui470 -llibopencv_imgcodecs470 -lopencv_imgproc470 \
#                                                 -llibopencv_calib3d470 -llibopencv_ml470 -llibopencv_objdetect470 -llibopencv_photo470 -llibopencv_dnn470 \
#                                                 -llibopencv_features2d470 -llibopencv_video470 -llibopencv_videoio470
# else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/build_opencv470/install/x64/mingw/bin/ -llibopencv_core470 \
#                                                 -llibopencv_highgui470 -llibopencv_imgcodecs470 -lopencv_imgproc470 \
#                                                 -llibopencv_calib3d470 -llibopencv_ml470 -llibopencv_objdetect470 -llibopencv_photo470 -llibopencv_dnn470 \
#                                                 -llibopencv_features2d470 -llibopencv_video470 -llibopencv_videoio470

# INCLUDEPATH += $$PWD/../../../../OpenCV/build_opencv470/install/include
# DEPENDPATH += $$PWD/../../../../OpenCV/build_opencv470/install/include


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/CMake_OpenCV/install/lib/ -lopencv_core455 -lopencv_highgui455 \
                                                -lopencv_imgcodecs455 -lopencv_imgproc455 \
                                                -lopencv_calib3d455 -lopencv_ml455 -lopencv_objdetect455 -lopencv_photo455 -lopencv_dnn455 \
                                                -lopencv_features2d455 -lopencv_video455 -lopencv_videoio455
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/CMake_OpenCV/install/lib/ -lopencv_core455 -lopencv_highgui455 \
                                                -lopencv_imgcodecs455 -lopencv_imgproc455 \
                                                -lopencv_calib3d455 -lopencv_ml455 -lopencv_objdetect455 -lopencv_photo455 -lopencv_dnn455 \
                                                -lopencv_features2d455 -lopencv_video455 -lopencv_videoio455

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/CMake_OpenCV/install/bin/ -lopencv_core455 -lopencv_highgui455 \
                                                -lopencv_imgcodecs455 -lopencv_imgproc455 \
                                                -lopencv_calib3d455 -lopencv_ml455 -lopencv_objdetect455 -lopencv_photo455 -lopencv_dnn455 \
                                                -lopencv_features2d455 -lopencv_video455 -lopencv_videoio455
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/CMake_OpenCV/install/bin/ -lopencv_core455 -lopencv_highgui455 \
                                                -lopencv_imgcodecs455 -lopencv_imgproc455 \
                                                -lopencv_calib3d455 -lopencv_ml455 -lopencv_objdetect455 -lopencv_photo455 -lopencv_dnn455 \
                                                -lopencv_features2d455 -lopencv_video455 -lopencv_videoio455

INCLUDEPATH += $$PWD/../../../../OpenCV/CMake_OpenCV/install/include
DEPENDPATH += $$PWD/../../../../OpenCV/CMake_OpenCV/install/include

RESOURCES += \
    resources.qrc


# win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV/software/opencv/build/x64/vc16/lib/ -lopencv_world4100
# else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV/software/opencv/build/x64/vc16/lib/ -lopencv_world4100

# INCLUDEPATH += $$PWD/../../../../OpenCV/software/opencv/build/include
# DEPENDPATH += $$PWD/../../../../OpenCV/software/opencv/build/include
