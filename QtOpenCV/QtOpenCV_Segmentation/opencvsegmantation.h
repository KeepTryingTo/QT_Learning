#ifndef OPENCVSEGMANTATION_H
#define OPENCVSEGMANTATION_H

#include <QMainWindow>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/video/video.hpp>

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
class OpenCVSegmantation;
}
QT_END_NAMESPACE

class OpenCVSegmantation : public QMainWindow
{
    Q_OBJECT

public:
    OpenCVSegmantation(QWidget *parent = nullptr);
    ~OpenCVSegmantation();

    QImage cvMatToQImage(const cv::Mat &inMat);
    void segmentImage(int height,int width);

    cv::Mat normalizeBlob(cv::Mat& inputBlob, cv::Scalar & mean, cv::Scalar & std);
    cv::Mat cam_mask(cv::Mat & mask,int num_classes);

private slots:
    void on_btn_open_image_clicked();

    void on_btn_open_model_clicked();

    void on_btn_video_clicked();

    void on_btn_close_clicked();

    void on_btn_segment_clicked();

private:
    Ui::OpenCVSegmantation *ui;

    cv::Mat Image;
    cv::dnn::Net model;
    QString modelPath;
    QMap<int,std::vector<int>>indexMapColor;
};
#endif // OPENCVSEGMANTATION_H
