#ifndef OBJECTDETECTIONNCNN_H
#define OBJECTDETECTIONNCNN_H

#include <QMainWindow>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/videoio.hpp>

// #include <ncnn/simpleocv.h>
#include <ncnn/net.h>


#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSize>
#include <QPixmap>
#include <QTime>
#include <QDateTime>
#include <QString>
#include <QMap>
#include <QVector>
#include <QTextStream>
#include <QList>


QT_BEGIN_NAMESPACE
namespace Ui {
class ObjectDetectionNCNN;
}
QT_END_NAMESPACE

class ObjectDetectionNCNN : public QMainWindow
{
    Q_OBJECT

public:
    ObjectDetectionNCNN(QWidget *parent = nullptr);
    ~ObjectDetectionNCNN();

    QImage cvMatToQImage(const cv::Mat &inMat);
    cv::Rect out2org(cv::Rect box, cv::Size crop_size,cv::Size org_size);
    void DetectImage(int height,int width);
    void readClassesFile(const QString filePath);

private slots:
    void on_btn_open_image_clicked();

    void on_btn_open_model_clicked();

    void on_btn_video_clicked();

    void on_btn_close_clicked();

    void on_btn_detect_clicked();

private:
    Ui::ObjectDetectionNCNN *ui;

    ncnn::Net model;

    cv::Mat Image;
    QString modelPath;
    QList<QString>mClasses;
    QMap<int,QString> indexMapName;
};
#endif // OBJECTDETECTIONNCNN_H
