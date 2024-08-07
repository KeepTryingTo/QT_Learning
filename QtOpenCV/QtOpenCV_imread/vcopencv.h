#ifndef VCOPENCV_H
#define VCOPENCV_H

#include <QMainWindow>

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

private slots:
    void on_btn_open_file_clicked();

    void on_btn_close_clicked();

private:
    Ui::VCOpenCV *ui;
};
#endif // VCOPENCV_H



