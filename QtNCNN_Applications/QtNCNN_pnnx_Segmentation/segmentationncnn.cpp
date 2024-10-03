#include "segmentationncnn.h"
#include "ui_segmentationncnn.h"

SegmentationNCNN::SegmentationNCNN(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SegmentationNCNN)
{
    ui->setupUi(this);

    this -> mClasses = 21;

    qDebug()<<"OpenCV Version: "<<CV_VERSION;
    qDebug()<<"OpenCV Major Version: "<<CV_VERSION_MAJOR;
    qDebug()<<"OpenCV Minor Version: "<<CV_VERSION_MINOR;
    //根据不同模型输入分辨率大小不同，设置图像分辨率
    ui -> comboBox_resolution -> addItem(QString("%1 x %2").arg(320).arg(320));
    ui -> comboBox_resolution -> addItem(QString("%1 x %2").arg(512).arg(512));
    ui -> comboBox_resolution -> addItem(QString("%1 x %2").arg(640).arg(640));

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

SegmentationNCNN::~SegmentationNCNN()
{
    delete ui;
}

QImage SegmentationNCNN::cvMatToQImage(const cv::Mat &inMat) {
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


cv::Mat SegmentationNCNN::cam_mask(cv::Mat &mask, int num_classes)
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

cv::Mat SegmentationNCNN::convertNcnnMatToCvMat(const ncnn::Mat& ncnnMat) {
    // 创建空的 cv::Mat,并且是21通道的
    cv::Mat cvMat(ncnnMat.h, ncnnMat.w, CV_32FC(21));
    int h = ncnnMat.h;
    int w = ncnnMat.w;
    int num_classes = this -> mClasses;
    qDebug()<<"cvMat.dims = "<<cvMat.dims;

    // 将数据从 ncnn::Mat 复制到 cv::Mat
    for (int c = 0; c < num_classes; c++) {
         // ptr 指向当前通道的起始位置
        float* dst = cvMat.ptr<float>(0, c);
         // 获取ncnn::Mat中对应通道的数据
        const float* src = ncnnMat.channel(c);

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                // 注意：ncnn::Mat 的存储是以通道优先的顺序，而 cv::Mat 是以行优先的顺序
                cvMat.at<cv::Vec<float,21>>(y,x)[c] = src[y * ncnnMat.w + x];
            }
        }
    }

    return cvMat;
}

void SegmentationNCNN::segmentImage(int height,int width){
    const float mean_vals[3] = {0.0f, 0.0f, 0.0f};
    const float norm_vals[3] = {1 / 255.0f, 1 / 255.0f, 1 / 255.0f};

    int img_w = this -> Image.cols;
    int img_h = this -> Image.rows;

    ncnn::Mat in = ncnn::Mat::from_pixels_resize(this -> Image.data,ncnn::Mat::PIXEL_BGR2RGB,
                                                 img_w,img_h,width,height);
    in.substract_mean_normalize(mean_vals,norm_vals);

    //前向推理
    ncnn::Extractor ex = this -> model.create_extractor();
    ex.input("in0",in);

    ncnn::Mat out;
    ex.extract("out0",out);

    qDebug()<<"out.dims = "<<out.dims;

    cv::Mat output = convertNcnnMatToCvMat(out);
    //根据对图像每一个像素预测的结果，选择最大概率值的类别索引
    cv::Mat mask = cv::Mat::zeros(cv::Size(width,height),CV_32S);
    qDebug()<<"dims = "<<output.dims<<" shape = "<<output.channels()<<" "<<output.size[0]<<" "<<output.size[1];
    int N = 1;
    int num_classes = output.channels();
    int H = output.size[0];
    int W = output.size[1];

    for(int i = 0 ; i < N; i++){
        for(int h = 0 ; h < H; h ++){
            for(int w = 0 ; w < W; w++){
                int classId = 0;
                // 获取当前像素的所有通道（类别概率）
                cv::Vec<float, 21> probabilities = output.at<cv::Vec<float, 21>>(h, w);
                float maxValue = probabilities[0];
                for (int c = 1; c < this -> mClasses; ++c) {
                    if (probabilities[c] > maxValue) {
                        maxValue = probabilities[c];
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

void SegmentationNCNN::on_btn_open_image_clicked(){
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


void SegmentationNCNN::on_btn_open_model_clicked(){
    QString model_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(model_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    this -> modelPath = model_path;

    //加载模型
    if(this -> model.load_param(this -> modelPath.toStdString().c_str())){
        QMessageBox::information(this,"提示","模型param加载失败");
        return;
    }
    if(this -> model.load_model(QString(this -> modelPath.split(".")[0] + ".ncnn.bin").toStdString().c_str())){
        QMessageBox::information(this,"提示","模型bin加载失败");
        return;
    }
}


void SegmentationNCNN::on_btn_video_clicked(){
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


void SegmentationNCNN::on_btn_close_clicked()
{
    this -> close();
}


void SegmentationNCNN::on_btn_segment_clicked()
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


