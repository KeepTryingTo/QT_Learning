#include "opencvdetection.h"
#include "ui_opencvdetection.h"

OpenCVDetection::OpenCVDetection(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OpenCVDetection)
{
    ui->setupUi(this);

    qDebug()<<"OpenCV Version: "<<CV_VERSION;
    qDebug()<<"OpenCV Major Version: "<<CV_VERSION_MAJOR;
    qDebug()<<"OpenCV Minor Version: "<<CV_VERSION_MINOR;
    //根据不同模型输入分辨率大小不同，设置图像分辨率
    ui -> comboBox -> addItem(QString("%1 x %2").arg(320).arg(320));
    ui -> comboBox -> addItem(QString("%1 x %2").arg(640).arg(640));
    ui -> comboBox -> addItem(QString("%1 x %2").arg(800).arg(800));

    //读取类别文件
    QFile file(":/new/prefix1/resources/classes.txt");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug()<<"打开类别文件失败";
        return;
    }
    QTextStream in(&file);//用于处理文本内容
    QString line;
    int i = 0;
    while(!in.atEnd()){//逐行读取文件内容（因为每一行代表一个类别）
        line = in.readLine().trimmed();
        this -> indexMapName[i] = line;
        i ++;
    }
}

OpenCVDetection::~OpenCVDetection()
{
    delete ui;
}

QImage OpenCVDetection::cvMatToQImage(const cv::Mat &inMat) {
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

void OpenCVDetection::on_btn_open_image_clicked(){
    QString image_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(image_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    QFileInfo fileinformation(image_path);

    this -> Image = cv::imread(image_path.toStdString());
    if(this -> Image.empty()){
        QMessageBox::information(this,"提示","打开图像失败");
        return;
    }

    qDebug()<<"height = "<<this -> Image.rows<<" width = "<<this -> Image.cols;

    //将当前读取的图像缩放到和显示图像的布局大小相同
    QSize labSize = ui -> label_image -> size();
    cv::resize(this -> Image,this -> Image,cv::Size(labSize.rwidth(),labSize.rheight()));

    QImage image = this -> cvMatToQImage(this -> Image);
    ui -> label_image -> setPixmap(QPixmap::fromImage(image));
}

void OpenCVDetection::on_btn_open_model_clicked(){
    QString model_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(model_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    this -> modelPath = model_path;

    //加载权重文件(ONNX文件路径)
    this -> model = cv::dnn::readNetFromONNX(this -> modelPath.toStdString());
    if(this -> model.empty()){
        QMessageBox::information(this,"提示","加载文件失败");
        return;
    }
}

cv::Rect OpenCVDetection::out2org(cv::Rect box, cv::Size crop_size,cv::Size org_size){
    double xleft = box.x;
    double yleft = box.y;
    double xright = box.x + box.width;
    double yright = box.y + box.height;

    xleft  = xleft / crop_size.width * org_size.width;
    xright =  xright / crop_size.width * org_size.width;
    yleft  = yleft / crop_size.height * org_size.height;
    yright = yright / crop_size.height * org_size.height;

    cv::Rect box_;
    box_.x = xleft;
    box_.y = yleft;
    box_.width = xright - xleft;
    box_.height = yright - yleft;

    return box_;
}

void OpenCVDetection::DetectImage(int height,int width){
    //对图像进行预处理，让输入的图像符合加载模型要求 => [N,C,H,W]
    cv::Mat blob;
    cv::dnn::blobFromImage(this -> Image,blob,1.0 / 255,cv::Size(height,width),cv::Scalar(),true,false);

    // qDebug()<<"blob.shape: "<<blob.size[0]<<","<<blob.size[1]<<","<<blob.size[2]<<","<<blob.size[3];
    // qDebug()<<"blob channels = "<<blob.channels();
    // qDebug()<<"blob total = "<<blob.total();

    this -> model.setInput(blob);
    //注意这里的输出层名称一定要和转换ONNX时指定的输出层名称相同
    std::vector<cv::Mat> outputBlobs;
    std::vector<std::string> outBlobNames = {"output0"};//注意这里的输出名称要和转换的ONNX模型文件对应

    this -> model.forward(outputBlobs,outBlobNames);
    // this -> model.forward(outputBlobs,model.getUnconnectedOutLayersNames());

    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;
    std::vector<int> classIds;

    float *data = (float *)outputBlobs[0].data;
    const int rows = 25200;

    for (int i = 0; i < rows; ++i) {
        float confidence = data[4];
        if (confidence >= ui -> spinbox_conf_threshold -> value()) {

            float * classes_scores = data + 5;
            cv::Mat scores(1, this -> indexMapName.size(), CV_32FC1, classes_scores);
            cv::Point class_id;
            double max_class_score;
            minMaxLoc(scores, 0, &max_class_score, 0, &class_id);

            confidences.push_back(confidence);
            classIds.push_back(class_id.x);

            float x = data[0];
            float y = data[1];
            float w = data[2];
            float h = data[3];
            float xleft = x - w / 2;
            float yleft = y - h / 2;
            boxes.push_back(cv::Rect(xleft,yleft, w, h));

        }
        data += 85;
    }
    qDebug()<<"box size: "<<boxes.size();

    //NMS算法过滤掉重叠的框
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, ui -> spinbox_conf_threshold -> value(),ui -> spinbox_iou_threshold -> value(), indices);
    //绘制检测信息到图像中
    for (int i : indices) {
        // 绘制有效的边界框
        cv::Rect box = boxes[i];
        //由于这里得到的边界框是相对于模型输入大小的，但是需要实际图像大小对坐标框进行调整
        box = this -> out2org(box,cv::Size(width,height),cv::Size(this -> Image.cols,this -> Image.rows));
        //绘制坐标框
        cv::rectangle(this -> Image, box, cv::Scalar(0, 255, 0), 2);
        //标上置信度以及类别
        QString text = QString("%1 %2%").arg(this -> indexMapName[classIds[i]],QString::number(confidences[i] * 100,'f',2));
        cv::Point org(box.x,box.y - 5);
        cv::putText(this -> Image,text.toStdString(),org,cv::FONT_HERSHEY_SIMPLEX,0.5,cv::Scalar(0,255,0),1);
    }
}

void OpenCVDetection::on_btn_detect_clicked(){
    if(this -> Image.empty() || this -> modelPath.isEmpty()){
        QMessageBox::information(this,"提示","未加载图像或者模型");
        return;
    }
    //开始预测时间
    QDateTime curr = QDateTime::currentDateTime();
    qint64 startTime = curr.toMSecsSinceEpoch();

    //得到当前模型输入的图像分辨率大小
    QString resolution = ui -> comboBox -> currentText();
    int height = resolution.split("x")[0].trimmed().toInt();
    int width = resolution.split("x")[1].trimmed().toInt();

    //绘制坐标框信息到原图中
    this -> DetectImage(height,width);
    QImage image = this -> cvMatToQImage(this -> Image);
    ui -> label_image -> setPixmap(QPixmap::fromImage(image));

    //推理结束时间
    qint64 endTime = curr.toMSecsSinceEpoch();
    ui -> label_time -> setText(QString::number(endTime - startTime) + "ms");
}
//opencv打开摄像头进行检测
void OpenCVDetection::on_btn_video_clicked(){
    cv::VideoCapture cap;
    cap.open(0);//0-打开默认的摄像头
    int count_fps = 0;

    //开始预测时间
    auto start = std::chrono::high_resolution_clock::now();

    //得到当前模型输入的图像分辨率大小
    QString resolution = ui -> comboBox -> currentText();
    int height = resolution.split("x")[0].trimmed().toInt();
    int width = resolution.split("x")[1].trimmed().toInt();

    //打开摄像头
    while(cap.isOpened()){
        cv::Mat frame;
        //读取视频帧并判断读取是否成功
        bool ret = cap.read(frame);
        if(!ret){
            QMessageBox::information(this,"提示","打开摄像头失败");
            return;
        }

        this -> Image = frame;

        //计算FPS
        auto end = std::chrono::high_resolution_clock::now();
        count_fps += 1;
        float fps = 0;
        int timeDist = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        if(timeDist > 0){
            fps = count_fps * 1000.0 / timeDist;
        }
        ui -> label_fps -> setText(QString::number(fps,'f',2));

        //对单张帧进行目标检测
        this -> DetectImage(height,width);

        frame = this -> Image;
        cv::imshow("img",frame);
        if(cv::waitKey(1) == 27){
            cap.release();
            cv::destroyAllWindows();
            break;
        }
    }
}

void OpenCVDetection::on_btn_close_clicked(){
    this -> close();
}




