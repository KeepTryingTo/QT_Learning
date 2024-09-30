#ifndef CLASSIFICATIONNCNN_V2_H
#define CLASSIFICATIONNCNN_V2_H

#include <QMainWindow>

// #include <ncnn/simpleocv.h>
#include <ncnn/net.h>

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
#include <cmath>
#include <cfloat>


enum ImreadModes
{
    IMREAD_UNCHANGED = -1,
    IMREAD_GRAYSCALE = 0,
    IMREAD_COLOR = 1
};

QT_BEGIN_NAMESPACE
namespace Ui {
class ClassificationNCNN_v2;
}
QT_END_NAMESPACE

class ClassificationNCNN_v2 : public QMainWindow
{
    Q_OBJECT

public:
    ClassificationNCNN_v2(QWidget *parent = nullptr);
    ~ClassificationNCNN_v2();

    QImage cvMatToQImage(const cv::Mat &inMat);
    QList<QString> readClassesFromFile(const QString &filePath);
    int detect_net(const cv::Mat& bgr, std::vector<float>& cls_scores);
    std::vector<float> softMax(std::vector<float> cls_scores);
    int print_topk(const std::vector<float>& cls_scores, int topk);

private slots:
    void on_btn_open_file_clicked();

    void on_btn_open_model_clicked();

    void on_btn_predict_clicked();

    void on_btn_close_clicked();

private:
    Ui::ClassificationNCNN_v2 *ui;

    QString model_name;
    cv::Mat Image;
    QString modelPath;
    QString imgPath;
    QList<QString>mClasses;
    QMap<int,QString>indexMapName;
};
#endif // CLASSIFICATIONNCNN_V2_H
