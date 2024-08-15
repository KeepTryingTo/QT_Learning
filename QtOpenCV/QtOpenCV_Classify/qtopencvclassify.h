#ifndef QTOPENCVCLASSIFY_H
#define QTOPENCVCLASSIFY_H

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
class QtOpenCVClassify;
}
QT_END_NAMESPACE

class QtOpenCVClassify : public QMainWindow
{
    Q_OBJECT

public:
    QtOpenCVClassify(QWidget *parent = nullptr);
    ~QtOpenCVClassify();

    QImage cvMatToQImage(const cv::Mat &inMat);
    cv::Mat createBatch(cv::Mat& img, int N);
    cv::Mat normalizeBlob(cv::Mat& inputBlob, cv::Scalar & mean, cv::Scalar & std);

private slots:
    void on_btn_open_file_clicked();

    void on_btn_open_model_clicked();

    void on_btn_predict_clicked();

    void on_btn_close_clicked();

private:
    Ui::QtOpenCVClassify *ui;

    cv::Mat Image;
    QString modelPath;
    QMap<int,QString>indexMapName;
};
#endif // QTOPENCVCLASSIFY_H
