﻿#include "vcopencv.h"
#include "ui_vcopencv.h"

VCOpenCV::VCOpenCV(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::VCOpenCV)
{
    ui->setupUi(this);

    ui -> spinBox -> setMaximum(255);
    ui -> horizontalSlider -> setMaximum(255);

    //初始化阈值的几种类型
    ui -> comboBox_threshold_type -> addItem("<None>");
    ui -> comboBox_threshold_type -> addItem("Binary Threshold");
    ui -> comboBox_threshold_type -> addItem("Binary Threshold Inverse");
    ui -> comboBox_threshold_type -> addItem("Trunc Threshold");
    ui -> comboBox_threshold_type -> addItem("Binary To Zero");
    ui -> comboBox_threshold_type -> addItem("Binary to Zero Inverse");

    //当spinBox的数值发生变化的时候，对应滑动条的值也要发送变化
    connect(ui -> spinBox,SIGNAL(valueChanged(int)),ui -> horizontalSlider,SLOT(setValue(int)));
    //初始化绘制的形状
    ui -> comboBox_shape -> addItem("<None>");
    ui -> comboBox_shape -> addItem("Rectangle");
    ui -> comboBox_shape -> addItem("Circle");
    ui -> comboBox_shape -> addItem("Line");
    ui -> comboBox_shape -> addItem("putText");

    connect(ui -> comboBox_shape,&QComboBox::currentIndexChanged,this,&VCOpenCV::onDrawShape);

    //初始化加入的风格方式
    ui -> comboBox_style -> addItem("add");
    ui -> comboBox_style -> addItem("not_add");
    ui -> comboBox_style -> addItem("subtract");
    ui -> comboBox_style -> addItem("not_subtract");

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
    QString image_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(image_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    QFileInfo fileinformation(image_path);

    this -> Image = cv::imread(image_path.toStdString());
    this -> resetImage = this -> Image;
    // cv::Mat threshold_image;
    // this -> threshold_type = this -> selectThreshold_type();
    // cv::threshold(this -> Image,
    //               threshold_image,
    //               ui -> spinBox -> value(),
    //               ui -> spinBox -> maximum(),
    //               this -> threshold_type);
    QImage qtImage = this -> cvMatToQImage(this -> Image);
    ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
    //显示图像的高宽以及文件名
    ui -> label_name -> setText(fileinformation.fileName().split(".")[0]);
    ui -> label_high -> setText(QString("高: %1").arg(QString::number(Image.rows)));
    ui -> label_width -> setText(QString("宽: %1").arg(QString::number(Image.cols)));
}


QImage VCOpenCV::cvMatToQImage(const cv::Mat &inMat) {
    switch (inMat.type()) {
    // 8-bit, 4 channel
    case CV_8UC4: {
        QImage image(
            inMat.data,//图像数据
            inMat.cols,//图像宽度
            inMat.rows,//图像高度
            static_cast<int>(inMat.step),//图像矩阵元素类型
            QImage::Format_ARGB32//图像的像素格式
            );
        return image;
    }

    // 8-bit, 3 channel
    case CV_8UC3: {
        QImage image(inMat.data,
                     inMat.cols,
                     inMat.rows,
                     static_cast<int>(inMat.step),
                     QImage::Format_RGB888);
        return image.rgbSwapped(); //彩色图像为RGB三通道，交换R和B通道
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
        QImage image(inMat.data,
                     inMat.cols,
                     inMat.rows,
                     static_cast<int>(inMat.step),
                     QImage::Format_Indexed8);
        image.setColorTable(sColorTable);
        return image;
    }

    default:
        break;
    }

    return QImage();
}

int VCOpenCV::selectThreshold_type()
{
    return ui -> comboBox_threshold_type -> currentIndex();
}

void VCOpenCV::on_btn_close_clicked()
{
    this -> close();
}

void VCOpenCV::on_horizontalSlider_valueChanged(int value)
{
    ui -> spinBox -> setValue(value);
    if(ui -> comboBox_threshold_type -> currentIndex() == 0){
        QMessageBox::information(this,"提示","请选择阈值类型");
        return;
    }
    cv::Mat threshold_image;
    this -> threshold_type = this -> selectThreshold_type();
    cv::threshold(this -> Image,threshold_image,value,ui -> spinBox -> maximum(),this -> threshold_type);
    QImage qtImage = this -> cvMatToQImage(threshold_image);
    ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
}

void VCOpenCV::mousePressEvent(QMouseEvent * event){//鼠标按下事件
    if(event -> button() == Qt::LeftButton){
        QPointF pos = event -> localPos();//获取相对控件的鼠标点击坐标位置
        this -> startPoint.x = pos.x();
        this -> startPoint.y = pos.y();
    }
    qDebug()<<"start point" <<this -> startPoint.x<<" "<<this -> startPoint.y;
}

void VCOpenCV::mouseReleaseEvent(QMouseEvent *event){//鼠标弹起事件
    QPointF pos = event -> localPos();//获取相对控件的鼠标点击坐标位置
    this -> endPoint.x = pos.x();
    this -> endPoint.y = pos.y();

    qDebug()<<"end point" <<this -> endPoint.x<<" "<<this -> endPoint.y;

    this -> onDrawShape(ui -> comboBox_shape -> currentIndex());

    if(ui -> checkBox -> isChecked()){
        this -> CropImage();
    }

    this -> startPoint = cv::Point(0,0);
    this -> endPoint = cv::Point(0,0);

}
void VCOpenCV::mouseMoveEvent(QMouseEvent * event){
    if(event -> button() && Qt::LeftButton){
        this -> update();
    }
}

void VCOpenCV::onDrawShape(int index)
{
    //首先创建一个全0的矩阵(宽度，高度)
    cv::Mat drawImage = cv::Mat::zeros(cv::Size(410,371),CV_8UC3);
    switch(index){
    case 1:{
        //创建一个矩阵框
        cv::Rect Rectangle;
        Rectangle.x = this -> startPoint.x;
        Rectangle.y = this -> startPoint.y;
        Rectangle.width = this -> endPoint.x - this -> startPoint.x;
        Rectangle.height = this -> endPoint.y - this -> startPoint.y;

        cv::rectangle(drawImage,Rectangle,cv::Scalar(0,255,0),2);
        QImage qtImage = this -> cvMatToQImage(drawImage);
        ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
        break;
    }
    case 2:{
        cv::Point center(60,60);
        int radius = 30;
        cv::circle(drawImage,center,radius,cv::Scalar(0,255,0),2);
        QImage qtImage = this -> cvMatToQImage(drawImage);
        ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
        break;
    }
    case 3:{
        cv::Point startPoint = this -> startPoint;
        cv::Point endPoint = this -> endPoint;
        cv::line(drawImage,startPoint,endPoint,cv::Scalar(0,255,0),2);
        QImage qtImage = this -> cvMatToQImage(drawImage);
        ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
        break;
    }
    case 4:{
        const cv::String text = "This is a demo";
        cv::Point org(30,30);
        int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        double fontScale = 1.0;
        cv::putText(drawImage,text,org,fontFace,fontScale,cv::Scalar(0,255,0),2);
        QImage qtImage = this -> cvMatToQImage(drawImage);
        ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
        break;
    }
    default:
        return;
    }
}


void VCOpenCV::on_spinBox_2_valueChanged(int arg1)
{
    int Angle = arg1;
    cv::Point2f Center(this -> Image.cols / 2,this -> Image.rows / 2);
    cv::Mat RotatedImage = cv::getRotationMatrix2D(Center,Angle,1.0);//得到旋转之后的图像
    //表示非轴对齐的矩形，即定义一个旋转矩形;boundingRect2f 返回包含旋转矩形的最小(精确)浮点矩形，不打算用于图像
    cv::Rect2f BoundingBox = cv::RotatedRect(cv::Point2f(),this -> Image.size(),Angle).boundingRect2f();

    qDebug()<<"before tx = "<<RotatedImage.at<double>(0,2)<<" ty = "<<RotatedImage.at<double>(1,2);
    RotatedImage.at<double>(0,2) += BoundingBox.width / 2.0 - this -> Image.cols / 2.0;
    RotatedImage.at<double>(1,2) += BoundingBox.height / 2.0 - this -> Image.rows / 2.0;

    qDebug()<<"a = "<<RotatedImage.at<double>(0,0)<<" c = "<<RotatedImage.at<double>(1,0);
    qDebug()<<"b = "<<RotatedImage.at<double>(0,1)<<" d = "<<RotatedImage.at<double>(1,1);
    qDebug()<<"after tx = "<<RotatedImage.at<double>(0,2)<<" ty = "<<RotatedImage.at<double>(1,2);
    qDebug()<<"BoundingBox.width / 2 = "<<BoundingBox.width / 2.0<<" BoundingBox.height / 2.0 = "<<BoundingBox.height / 2.0;

    cv::Mat DestinationImage;
    //https://docs.opencv.org/4.5.5/da/d54/group__imgproc__transform.html#ga0203d9ee5fcd28d40dbc4a1ea4451983
    cv::warpAffine(this -> Image,DestinationImage,RotatedImage,BoundingBox.size(),1,0,cv::Scalar(255,255,255));

    QImage qtImage = this -> cvMatToQImage(DestinationImage);
    ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
}


void VCOpenCV::on_doubleSpinBox_valueChanged(double arg1)
{
    double scale_ratio = ui -> doubleSpinBox -> value();
    int new_width = (int)(this -> Image.cols * scale_ratio);
    int new_height = (int)(this -> Image.rows * scale_ratio);

    cv::Mat DestinationImage;
    cv::resize(this -> Image,DestinationImage,cv::Size(new_width,new_height));

    QImage qtImage = this -> cvMatToQImage(DestinationImage);
    ui -> label -> setPixmap(QPixmap::fromImage(qtImage));

    // this -> Image = DestinationImage;
}

void VCOpenCV::CropImage(){
    cv::Rect cropSize;
    cropSize.x = this -> startPoint.x;
    cropSize.y = this -> startPoint.y;
    cropSize.width = this -> endPoint.x - this -> startPoint.x;
    cropSize.height = this -> endPoint.y - this -> startPoint.y;

    cv::Mat DestinationImage = this -> Image(cropSize);

    QImage qtImage = this -> cvMatToQImage(DestinationImage);
    ui -> label -> setPixmap(QPixmap::fromImage(qtImage));

    this -> Image = DestinationImage;
}


void VCOpenCV::on_pushButton_clicked(){
    QString image_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(image_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    QFileInfo fileinformation(image_path);

    cv::Mat styleImage = cv::imread(image_path.toStdString());
    //将风格图像缩放到和原图大小相同
    int height = this -> Image.rows;
    int width = this -> Image.cols;
    cv::resize(styleImage,styleImage,cv::Size(width,height));//原地修改风格图大小

    cv::Mat mixed_styleImage_orgImage;
    switch(ui -> comboBox_style -> currentIndex()){
        case 0:{
                cv::add(this -> Image,styleImage,mixed_styleImage_orgImage);

                QImage qtImage = this -> cvMatToQImage(mixed_styleImage_orgImage);
                ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
                break;
            }
        case 1:{
                cv::bitwise_not(this -> Image,this -> Image);//原地修改原图，对其取反操作
                cv::add(this -> Image,styleImage,mixed_styleImage_orgImage);

                QImage qtImage = this -> cvMatToQImage(mixed_styleImage_orgImage);
                ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
                break;
            }
        case 2:{
            cv::subtract(this -> Image,styleImage,mixed_styleImage_orgImage);

            QImage qtImage = this -> cvMatToQImage(mixed_styleImage_orgImage);
            ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
            break;
        }
        case 3:{
            cv::bitwise_not(this -> Image,this -> Image);//原地修改原图，对其取反操作
            cv::add(this -> Image,styleImage,mixed_styleImage_orgImage);

            QImage qtImage = this -> cvMatToQImage(mixed_styleImage_orgImage);
            ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
            break;
        }
        default:
            break;
    }
}


void VCOpenCV::on_btn_reset_clicked()
{
    this -> Image = this -> resetImage;
    QImage qtImage = this -> cvMatToQImage(this -> Image);
    ui -> label -> setPixmap(QPixmap::fromImage(qtImage));
}

