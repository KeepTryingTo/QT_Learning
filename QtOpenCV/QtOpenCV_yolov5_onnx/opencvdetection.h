#ifndef OPENCVDETECTION_H
#define OPENCVDETECTION_H

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
class OpenCVDetection;
}
QT_END_NAMESPACE

class OpenCVDetection : public QMainWindow
{
    Q_OBJECT

public:
    OpenCVDetection(QWidget *parent = nullptr);
    ~OpenCVDetection();

    QImage cvMatToQImage(const cv::Mat &inMat);
    cv::Rect out2org(cv::Rect box, cv::Size crop_size,cv::Size org_size);
    void DetectImage(int height,int width);

private slots:
    void on_btn_open_image_clicked();

    void on_btn_open_model_clicked();

    void on_btn_video_clicked();

    void on_btn_close_clicked();

    void on_btn_detect_clicked();

private:
    Ui::OpenCVDetection *ui;

    cv::Mat Image;
    QString modelPath;
    cv::dnn::Net model;

    QMap<int,QString> indexMapName;
};
#endif // OPENCVDETECTION_H
