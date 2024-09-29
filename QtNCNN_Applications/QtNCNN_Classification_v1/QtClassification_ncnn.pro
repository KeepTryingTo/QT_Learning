QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
# CONFIG += openmp

# QMAKE_CXXFLAGS += -fopenmp
# QMAKE_LFLAGS += -fopenmp
# LIBS += -fopenmp -lgomp

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    classificationncnn.cpp

HEADERS += \
    classificationncnn.h

FORMS += \
    classificationncnn.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../conda3/Transfer_Learning/NCNN/installed_ncnn/lib/ -lncnn
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../conda3/Transfer_Learning/NCNN/installed_ncnn/lib/ -lncnn

INCLUDEPATH += $$PWD/../../../../../conda3/Transfer_Learning/NCNN/installed_ncnn/include
DEPENDPATH += $$PWD/../../../../../conda3/Transfer_Learning/NCNN/installed_ncnn/include


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


# win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../conda3/Transfer_Learning/NCNN/ncnn_20240820_windows_vs2022/x64/lib/ -lncnn
# else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../conda3/Transfer_Learning/NCNN/ncnn_20240820_windows_vs2022/x64/lib/ -lncnn

# INCLUDEPATH += $$PWD/../../../../../conda3/Transfer_Learning/NCNN/ncnn_20240820_windows_vs2022/x64/include
# DEPENDPATH += $$PWD/../../../../../conda3/Transfer_Learning/NCNN/ncnn_20240820_windows_vs2022/x64/include

RESOURCES += \
    resources.qrc
