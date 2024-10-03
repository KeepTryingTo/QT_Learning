#ifndef SEGMENTATIONNCNN_H
#define SEGMENTATIONNCNN_H

#include <QMainWindow>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/videoio.hpp>

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

QT_BEGIN_NAMESPACE
namespace Ui {
class SegmentationNCNN;
}
QT_END_NAMESPACE

class SegmentationNCNN : public QMainWindow
{
    Q_OBJECT

public:
    SegmentationNCNN(QWidget *parent = nullptr);
    ~SegmentationNCNN();

    QImage cvMatToQImage(const cv::Mat &inMat);
    void segmentImage(int height,int width);
    cv::Mat cam_mask(cv::Mat &mask, int num_classes);
    cv::Mat convertNcnnMatToCvMat(const ncnn::Mat& ncnnMat);


private slots:
    void on_btn_open_image_clicked();

    void on_btn_open_model_clicked();

    void on_btn_video_clicked();

    void on_btn_close_clicked();

    void on_btn_segment_clicked();

private:
    Ui::SegmentationNCNN *ui;

    cv::Mat Image;
    QString modelPath;
    ncnn::Net model;
    int mClasses = 0;
    QMap<int,std::vector<int>>indexMapColor;
};
#endif // SEGMENTATIONNCNN_H
