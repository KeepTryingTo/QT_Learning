#ifndef OPENCVDNN_H
#define OPENCVDNN_H

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

QT_BEGIN_NAMESPACE
namespace Ui {
class OpenCVDNN;
}
QT_END_NAMESPACE

class OpenCVDNN : public QMainWindow
{
    Q_OBJECT

public:
    OpenCVDNN(QWidget *parent = nullptr);
    ~OpenCVDNN();

    QImage cvMatToQImage(const cv::Mat &inMat);
    cv::Mat normalizeBlob(cv::Mat& inputBlob, cv::Scalar & mean, cv::Scalar & std);

private slots:
    void on_btn_open_file_clicked();

    void on_btn_predict_clicked();

    void on_btn_close_clicked();

    void on_btn_open_model_clicked();

private:
    Ui::OpenCVDNN *ui;

    cv::Mat Image;
    QString modelPath;
    QMap<int,QString>indexMapName;
    cv::dnn::Net model;
};
#endif // OPENCVDNN_H
