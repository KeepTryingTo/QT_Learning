#include "opencvdnn.h"
#include "ui_opencvdnn.h"

OpenCVDNN::OpenCVDNN(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OpenCVDNN)
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

OpenCVDNN::~OpenCVDNN(){
    delete ui;
}

QImage OpenCVDNN::cvMatToQImage(const cv::Mat &inMat) {
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

void OpenCVDNN::on_btn_open_file_clicked(){
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

cv::Mat OpenCVDNN::normalizeBlob(cv::Mat& inputBlob, cv::Scalar & mean, cv::Scalar & std) {
    // 确保输入是 4D Blob  [NCHW]
    if (inputBlob.dims != 4 || inputBlob.size[0] != 1 || inputBlob.size[1] != 3 || inputBlob.size[2] != 224 || inputBlob.size[3] != 224) {
        throw std::invalid_argument("Input blob must be of shape [1, 3, 224, 224]");
    }
    int N = inputBlob.size[0];
    int C = inputBlob.size[1];
    int H = inputBlob.size[2];
    int W = inputBlob.size[3];
    // 对每个通道进行均值和标准差的归一化
    for(int n = 0 ; n < N; n++){
        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < 224; ++h) {
                for (int w = 0; w < 224; ++w) {
                    //索引位置计算：
                    int index= n * (C*H*W)+c*(H*W)+h*W+w;
                    inputBlob.ptr<float>()[index] = (inputBlob.ptr<float>()[index] - mean[c]) / std[c];
                }
            }
        }
    }
    return inputBlob;
}

void OpenCVDNN::on_btn_predict_clicked(){
    //开始预测时间
    QDateTime curr = QDateTime::currentDateTime();
    qint64 startTime = curr.toMSecsSinceEpoch();

    //对图像进行预处理，让输入的图像符合加载模型要求 => [N,C,H,W]
    cv::Mat blob;
    // 设定均值和标准差（与 PyTorch 中相同）
    cv::Scalar mean(0.485, 0.456, 0.406);
    cv::Scalar std_dev(0.229, 0.224, 0.225);

    cv::dnn::blobFromImage(this -> Image,blob,1.0 / 255,cv::Size(224,224),cv::Scalar(),true,false);

    qDebug()<<"blob.shape: "<<blob.size[0]<<","<<blob.size[1]<<","<<blob.size[2]<<","<<blob.size[3];
    qDebug()<<"blob channels = "<<blob.channels();
    qDebug()<<"blob total = "<<blob.total();
    // 归一化处理
    blob = this -> normalizeBlob(blob,mean,std_dev);
    // for (int c = 0; c < blob.channels(); ++c) {
    //     float* data = blob.ptr<float>(c);
    //     for (int i = 0; i < blob.total(); ++i) {
    //         data[i] = (data[i] - mean[c]) / std_dev[c];  // 手动归一化
    //     }
    // }

    this -> model.setInput(blob);
    //注意这里的输出层名称一定要和转换ONNX时指定的输出层名称相同
    cv::Mat output = this -> model.forward("predictions");

# if 0
    //方式一：找到最大预测概率以及对应的索引
    cv::Point classID;
    double confidence;
    cv::minMaxLoc(output,0,&confidence,0,&classID);
    qDebug()<<"conf = "<<confidence<<" classid = "<<classID.x<<" name = "<<this -> indexMapName[classID.x];

    //方式二：找到最大预测概率以及对应的索引
    int num_classes = output.size[1];
    std::vector<double> class_probs;
    for (int i = 0; i < num_classes; ++i) {
        class_probs.push_back(output.at<float>(0, i));
        qDebug()<<"class probs = "<<output.at<float>(0, i);
    }

    // 找到概率最高的类别
    auto max_it = std::max_element(class_probs.begin(), class_probs.end());
    int class_id = std::distance(class_probs.begin(), max_it);
    qDebug() <<"confidence = "<<class_probs[class_id]<< " Predicted class: " << class_id;
# endif

    //方式三：找到最大预测概率以及对应的索引
    qDebug()<<"output.size: "<<output.rows<<","<<output.cols;
    double conf = 0;
    int index = 0;
    for(int j = 0 ; j < output.cols; j++){//遍历类别数
        qDebug()<<"conf = "<<output.at<float>(0,j);
        if(conf < output.at<float>(0,j)){
            conf = output.at<float>(0,j);
            index = j;
        }
    }
    qDebug()<<"conf = "<<conf<<" index = "<<index;

    //结束预测时间
    curr = QDateTime::currentDateTime();
    qint64 endTime = curr.toMSecsSinceEpoch();
    ui -> label_time -> setText(QString::number(endTime - startTime) + "ms");
    ui -> label_result -> setText(QString("%1 %2%").arg(this -> indexMapName[index]).arg(QString::number(conf * 100,'f',2)));
}

void OpenCVDNN::on_btn_close_clicked(){
    this -> close();
}

void OpenCVDNN::on_btn_open_model_clicked()
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
