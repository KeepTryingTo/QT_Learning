#include "vcopencv.h"
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

    Image = cv::imread(image_path.toStdString());
    cv::Mat threshold_image;
    this -> threshold_type = this -> selectThreshold_type();
    cv::threshold(this -> Image,
                  threshold_image,
                  ui -> spinBox -> value(),
                  ui -> spinBox -> maximum(),
                  this -> threshold_type);
    QImage qtImage = this -> cvMatToQImage(threshold_image);
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
    cv::Mat drawImage = cv::Mat::zeros(cv::Size(410,290),CV_8UC3);
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

