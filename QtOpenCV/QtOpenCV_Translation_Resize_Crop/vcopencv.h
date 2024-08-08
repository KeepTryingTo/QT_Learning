#ifndef VCOPENCV_H
#define VCOPENCV_H

#include <QMainWindow>

#include <QMessageBox>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QMouseEvent>
#include <QPoint>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>

QT_BEGIN_NAMESPACE
namespace Ui {
class VCOpenCV;
}
QT_END_NAMESPACE

class VCOpenCV : public QMainWindow
{
    Q_OBJECT

public:
    VCOpenCV(QWidget *parent = nullptr);
    ~VCOpenCV();

    QImage cvMatToQImage(const cv::Mat &inMat);
    int selectThreshold_type();
    void CropImage();
    void mousePressEvent(QMouseEvent * event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void on_btn_open_file_clicked();

    void on_btn_close_clicked();

    void on_horizontalSlider_valueChanged(int value);

    void onDrawShape(int index);

    void on_spinBox_2_valueChanged(int arg1);

    void on_doubleSpinBox_valueChanged(double arg1);

private:
    Ui::VCOpenCV *ui;

    int threshold_type = 0;
    cv::Point startPoint;
    cv::Point endPoint;
    cv::Point center;
    cv::Rect rectangle;

    cv::Mat Image;
};
#endif // VCOPENCV_H



