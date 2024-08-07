#include "vcopencv.h"
#include "ui_vcopencv.h"

#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>

VCOpenCV::VCOpenCV(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::VCOpenCV)
{
    ui->setupUi(this);

    //refer to: https://blog.csdn.net/nownow_/article/details/137913104

    qDebug()<<"OpenCV Version: "<<CV_VERSION;
    qDebug()<<"OpenCV Major Version: "<<CV_VERSION_MAJOR;
    qDebug()<<"OpenCV Minor Version: "<<CV_VERSION_MINOR;
}

VCOpenCV::~VCOpenCV()
{
    delete ui;
}

void VCOpenCV::on_btn_open_file_clicked()
{
    QString image_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("MP3 Files(*.mp3);;MP4 Files(*.mp4);;All Files(*.*)"));
    if(image_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    QFileInfo fileinformation(image_path);

    cv::Mat Image = cv::imread("D:\\SoftwareFamily\\QT\\projects\\QtOpenCV_Application\\QtOpenCV\\resources\\dog01.png");
    QImage qtImage = this -> cvMatToQImage(Image);
    ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
    //显示图像的高宽以及文件名
    ui -> label_name -> setText(fileinformation.fileName().split(".")[0]);
    ui -> label_high -> setText(QString("%1").arg(QString::number(Image.rows)));
    ui -> label_width -> setText(QString("%1").arg(QString::number(Image.cols)));
}


QImage VCOpenCV::cvMatToQImage(const cv::Mat &inMat) {
    switch (inMat.type()) {
    // 8-bit, 4 channel
    case CV_8UC4: {
        QImage image(inMat.data, inMat.cols, inMat.rows, static_cast<int>(inMat.step), QImage::Format_ARGB32);
        return image;
    }

    // 8-bit, 3 channel
    case CV_8UC3: {
        QImage image(inMat.data, inMat.cols, inMat.rows, static_cast<int>(inMat.step), QImage::Format_RGB888);
        return image.rgbSwapped();
    }

    // 8-bit, 1 channel
    case CV_8UC1: {
        static QVector<QRgb> sColorTable;
        // 我们只需要初始化一次颜色表
        if (sColorTable.isEmpty()) {
            for (int i = 0; i < 256; ++i) {
                sColorTable.push_back(qRgb(i, i, i));
            }
        }
        QImage image(inMat.data, inMat.cols, inMat.rows, static_cast<int>(inMat.step), QImage::Format_Indexed8);
        image.setColorTable(sColorTable);
        return image;
    }

    default:
        break;
    }

    return QImage();
}

void VCOpenCV::on_btn_close_clicked()
{
    this -> close();
}

