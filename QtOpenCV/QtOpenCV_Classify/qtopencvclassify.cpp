#include "qtopencvclassify.h"
#include "ui_qtopencvclassify.h"

QtOpenCVClassify::QtOpenCVClassify(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::QtOpenCVClassify)
{
    ui->setupUi(this);

    qDebug()<<"OpenCV Version: "<<CV_VERSION;
    qDebug()<<"OpenCV Major Version: "<<CV_VERSION_MAJOR;
    qDebug()<<"OpenCV Minor Version: "<<CV_VERSION_MINOR;

    //初始化索引到对应类别名称的映射 name = ['黛西','蒲公英','玫瑰','向日葵','郁金香']
    QList<QString> nameList = {"黛西","蒲公英","玫瑰","向日葵","郁金香"};
    for(int i = 0 ; i < nameList.size(); i++){
        this -> indexMapName[i] = nameList[i];
    }
}

QtOpenCVClassify::~QtOpenCVClassify()
{
    delete ui;
}

QImage QtOpenCVClassify::cvMatToQImage(const cv::Mat &inMat) {
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

void QtOpenCVClassify::on_btn_open_file_clicked(){
    QString image_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(image_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }

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
//增加维度
cv::Mat QtOpenCVClassify::createBatch(cv::Mat& img, int N) {
    // 获取原始图像的维度
    int channels = img.channels(); // 通道数
    int height = img.rows;         // 高度
    int width = img.cols;          // 宽度
    // 检查输入图像是否为空
    cv::Mat batch;
    if(img.empty()){
        throw std::invalid_argument("img is can not null");
        return cv::Mat();
    }
    if (img.dims != 3) {
        // 重新调整mat维度:2D [HW] => 3D [CHW]
        batch = img.reshape(1, std::vector<int>{channels, height, width});
        // 重新调整mat维度 3D [CHW] => 4D [NCHW]
        batch = batch.reshape(1, std::vector<int>{N, channels, height, width});
    }else{
        // 重新调整mat维度 3D [CHW] => 4D [NCHW]
        batch = img.reshape(1, std::vector<int>{N, channels, height, width});
    }
    return batch;
}
//归一化
cv::Mat QtOpenCVClassify::normalizeBlob(cv::Mat& inputBlob, cv::Scalar & mean, cv::Scalar & std) {

    int H = inputBlob.rows;
    int W = inputBlob.cols;
    // 对每个通道进行均值和标准差的归一化
    for (int i = 0; i < H; ++i) {
        for (int j = 0; j < W; ++j) {
            cv::Vec3f& pixel = inputBlob.at<cv::Vec3f>(i, j);
            pixel[0] = (pixel[0] - mean[0]) / std[0]; // 归一化 R 通道
            pixel[1] = (pixel[1] - mean[1]) / std[1]; // 归一化 G 通道
            pixel[2] = (pixel[2] - mean[2]) / std[2]; // 归一化 B 通道
        }
    }
    return inputBlob;
}


void QtOpenCVClassify::on_btn_predict_clicked(){
    //开始预测时间
    QDateTime curr = QDateTime::currentDateTime();
    qint64 startTime = curr.toMSecsSinceEpoch();

    //加载权重文件(ONNX文件路径)
    cv::dnn::Net net = cv::dnn::readNetFromONNX(this -> modelPath.toStdString());
    if(net.empty()){
        QMessageBox::information(this,"提示","加载文件失败");
        return;
    }

    cv::dnn::ClassificationModel model(net);

    // 设定均值和标准差（与 PyTorch 中相同）
    cv::Scalar mean(0.485, 0.456, 0.406);
    cv::Scalar std_dev(0.229, 0.224, 0.225);

    model.setInputSize(cv::Size(224,224));//缩放到指定大小
    // model.setInputScale(1.0 / 255);
    // model.setInputSwapRB(true);
    // model.setInputCrop(false);

    cv::resize(this -> Image,this -> Image,cv::Size(224,224));
    this -> Image.convertTo(this -> Image,CV_32F,1.0 / 255.0);//归一化到[0,255]
    cv::cvtColor(this -> Image, this -> Image, cv::COLOR_BGR2RGB);//交换R和B通道
    // this -> Image = this -> createBatch(this -> Image,1); //手动的升维，加一个维度batch = 1
    // qDebug()<<"Image.shape: "<<this -> Image.size[0]<<","<<this -> Image.size[1]<<","<<this -> Image.size[2]<<","<<this -> Image.size[3];
    // //归一化方式一
    this -> Image = this -> normalizeBlob(this -> Image,mean,std_dev); //手动归一化
    //归一化方式二
    // qDebug()<<"image channels = "<<this -> Image.channels();
    // qDebug()<<"image total = "<<this -> Image.total();
    // for (int c = 0; c < this -> Image.channels(); ++c) {
    //     float* data = this -> Image.ptr<float>(c);//ptr<T>(c) 函数返回通道 c 的起始地址
    //     for (int i = 0; i < this -> Image.total(); ++i) {
    //         data[i] = (data[i] - mean[c]) / std_dev[c];  // 手动归一化
    //     }
    // }

    cv::Mat output;
    std::pair<int, float> predictions = model.classify(this -> Image);
    qDebug()<<"index = "<<predictions.first <<" conf = "<<predictions.second;

    //结束预测时间
    curr = QDateTime::currentDateTime();
    qint64 endTime = curr.toMSecsSinceEpoch();
    ui -> label_time -> setText(QString::number(endTime - startTime) + "ms");
    ui -> label_result -> setText(QString("%1  %2%").arg(this -> indexMapName[predictions.first]).arg(QString::number(predictions.second * 100,'f',2)));
}

void QtOpenCVClassify::on_btn_close_clicked(){
    this -> close();
}

void QtOpenCVClassify::on_btn_open_model_clicked()
{
    QString model_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(model_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    this -> modelPath = model_path;

}
