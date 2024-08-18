#include "opencvsegmantation.h"
#include "ui_opencvsegmantation.h"

OpenCVSegmantation::OpenCVSegmantation(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OpenCVSegmantation)
{
    ui->setupUi(this);

    qDebug()<<"OpenCV Version: "<<CV_VERSION;
    qDebug()<<"OpenCV Major Version: "<<CV_VERSION_MAJOR;
    qDebug()<<"OpenCV Minor Version: "<<CV_VERSION_MINOR;
    //根据不同模型输入分辨率大小不同，设置图像分辨率
    ui -> comboBox_resolution -> addItem(QString("%1 x %2").arg(320).arg(320));
    ui -> comboBox_resolution -> addItem(QString("%1 x %2").arg(512).arg(512));
    ui -> comboBox_resolution -> addItem(QString("%1 x %2").arg(800).arg(800));

    //读取类别文件
    QFile file(":/new/prefix1/resources/platte.txt");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug()<<"打开类别文件失败";
        return;
    }
    QTextStream in(&file);//用于处理文本内容
    QString line;
    int i = 0;
    while(!in.atEnd()){//逐行读取文件内容（因为每一行代表一个类别）
        line = in.readLine();
        int R = line.split(",")[0].trimmed().toInt();
        int G = line.split(",")[1].trimmed().toInt();
        int B = line.split(",")[2].trimmed().toInt();
        std::vector<int>color_array = {R,G,B};
        this -> indexMapColor[i] = color_array;
        i ++;
    }

}

OpenCVSegmantation::~OpenCVSegmantation()
{
    delete ui;
}

QImage OpenCVSegmantation::cvMatToQImage(const cv::Mat &inMat) {
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


cv::Mat OpenCVSegmantation::normalizeBlob(cv::Mat& inputBlob, cv::Scalar & mean, cv::Scalar & std) {
    // 确保输入是 4D Blob  [NCHW]
    //得到当前模型输入的图像分辨率大小
    QString resolution = ui -> comboBox_resolution -> currentText();
    int height = resolution.split("x")[0].trimmed().toInt();
    int width = resolution.split("x")[1].trimmed().toInt();
    if (inputBlob.dims != 4 || inputBlob.size[0] != 1 || inputBlob.size[1] != 3 || inputBlob.size[2] != height || inputBlob.size[3] != width) {
        throw std::invalid_argument("Input blob must be of shape [1, 3, 224, 224]");
    }
    int N = inputBlob.size[0];
    int C = inputBlob.size[1];
    int H = inputBlob.size[2];
    int W = inputBlob.size[3];
    // 对每个通道进行均值和标准差的归一化
    for(int n = 0 ; n < N; n++){
        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < height; ++h) {
                for (int w = 0; w < width; ++w) {
                    //索引位置计算：
                    int index= n * (C*H*W)+c*(H*W)+h*W+w;
                    inputBlob.ptr<float>()[index] = (inputBlob.ptr<float>()[index] - mean[c]) / std[c];
                }
            }
        }
    }
    return inputBlob;
}

cv::Mat OpenCVSegmantation::cam_mask(cv::Mat &mask, int num_classes)
{
    //得到当前模型输入的图像分辨率大小
    QString resolution = ui -> comboBox_resolution -> currentText();
    int height = resolution.split("x")[0].trimmed().toInt();
    int width = resolution.split("x")[1].trimmed().toInt();
    //创建空矩阵用于保存分割之后的图
    cv::Mat seg_img = cv::Mat::zeros(cv::Size(width,height),CV_8UC3);

    for(int i = 0; i < num_classes; i++) {
        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                cv::Vec3b & pixel = seg_img.at<cv::Vec3b>(h,w); // (R,G,B)

                if(mask.at<int>(h,w) == i){
                    pixel[0] = cv::saturate_cast<uchar>(this -> indexMapColor[i][0]);
                    pixel[1] = cv::saturate_cast<uchar>(this -> indexMapColor[i][1]);
                    pixel[2] = cv::saturate_cast<uchar>(this -> indexMapColor[i][2]);
                }
            }
        }
    }
    return seg_img;
}

void OpenCVSegmantation::segmentImage(int height,int width)
{
    //对图像进行预处理，让输入的图像符合加载模型要求 => [N,C,H,W]
    cv::Mat blob;
    // 设定均值和标准差（与 PyTorch 中相同）
    cv::Scalar mean(0.485, 0.456, 0.406);
    cv::Scalar std_dev(0.229, 0.224, 0.225);
    cv::dnn::blobFromImage(this -> Image,blob,1.0 / 255,cv::Size(height,width),cv::Scalar(),true,false);

    qDebug()<<"blob.shape: "<<blob.size[0]<<","<<blob.size[1]<<","<<blob.size[2]<<","<<blob.size[3];
    qDebug()<<"blob channels = "<<blob.channels();
    qDebug()<<"blob total = "<<blob.total();

    blob = this -> normalizeBlob(blob,mean,std_dev);
    this -> model.setInput(blob);
    //注意这里的输出层名称一定要和转换ONNX时指定的输出层名称相同
    std::vector<cv::Mat> outputBlobs;
    std::vector<std::string> outBlobNames = {"out","aux"};//注意这里的输出名称要和转换的ONNX模型文件对应

    this -> model.forward(outputBlobs,outBlobNames);
    // this -> model.forward(outputBlobs,model.getUnconnectedOutLayersNames());

    qDebug()<<"outputBlobs.size: "<<outputBlobs.size();
    //out.shape:  1 , 21 , 512 , 512 aux.shape:  1 , 21 , 512 , 512
    qDebug()<<"out.shape: "<<outputBlobs[0].size[0]<<","<<outputBlobs[0].size[1]<<","<<outputBlobs[0].size[2]<<","<<outputBlobs[0].size[3];
    qDebug()<<"aux.shape: "<<outputBlobs[1].size[0]<<","<<outputBlobs[1].size[1]<<","<<outputBlobs[1].size[2]<<","<<outputBlobs[1].size[3];

    cv::Mat out = outputBlobs[0];
    cv::Mat aux = outputBlobs[1];
    //根据对图像每一个像素预测的结果，选择最大概率值的类别索引
    cv::Mat mask = cv::Mat::zeros(cv::Size(width,height),CV_32S);
    int N = outputBlobs[0].size[0];
    int num_classes = outputBlobs[0].size[1];
    int H = outputBlobs[0].size[2];
    int W = outputBlobs[0].size[3];

    for(int i = 0 ; i < N; i++){
        for(int h = 0 ; h < H; h ++){
            for(int w = 0 ; w < W; w++){
                float max_prob = 0;
                int classId = 0;
                for(int c = 0 ; c < num_classes; c++){
                    int index= i * (num_classes*H*W)+c*(H*W)+h*W+w;
                    if(out.ptr<float>()[index] > max_prob){
                        max_prob = out.ptr<float>()[index];
                        classId = c;
                    }
                }
                mask.at<int>(h,w) = classId;
            }
        }
    }

    cv::Mat seg_img = this -> cam_mask(mask,num_classes);
    int org_width = this -> Image.cols;
    int org_height = this -> Image.rows;
    cv::resize(seg_img,this -> Image,cv::Size(org_width,org_height));
}

void OpenCVSegmantation::on_btn_open_image_clicked()
{
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


void OpenCVSegmantation::on_btn_open_model_clicked()
{
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


void OpenCVSegmantation::on_btn_video_clicked()
{
    cv::VideoCapture cap;
    cap.open(0);//0-打开默认的摄像头
    int count_fps = 0;

    //开始预测时间
    auto start = std::chrono::high_resolution_clock::now();

    //得到当前模型输入的图像分辨率大小
    QString resolution = ui -> comboBox_resolution -> currentText();
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

        //对单张帧进行segment
        this -> segmentImage(height,width);

        frame = this -> Image;
        cv::imshow("img",frame);
        //按下ESC键结束检测
        if(cv::waitKey(1) == 27){
            cap.release();
            cv::destroyAllWindows();
            break;
        }
    }
}


void OpenCVSegmantation::on_btn_close_clicked()
{
    this -> close();
}


void OpenCVSegmantation::on_btn_segment_clicked()
{
    if(this -> Image.empty() || this -> modelPath.isEmpty()){
        QMessageBox::information(this,"提示","未加载图像或者模型");
        return;
    }
    //开始预测时间
    QDateTime curr = QDateTime::currentDateTime();
    qint64 startTime = curr.toMSecsSinceEpoch();

    //得到当前模型输入的图像分辨率大小
    QString resolution = ui -> comboBox_resolution -> currentText();
    int height = resolution.split("x")[0].trimmed().toInt();
    int width = resolution.split("x")[1].trimmed().toInt();

    //绘制坐标框信息到原图中
    this -> segmentImage(height,width);

    QImage image = this -> cvMatToQImage(this -> Image);
    ui -> label_image -> setPixmap(QPixmap::fromImage(image));

    //推理结束时间
    qint64 endTime = curr.toMSecsSinceEpoch();
    ui -> label_time -> setText(QString::number(endTime - startTime) + "ms");
}

