#include "classificationncnn.h"
#include "ui_classificationncnn.h"

ClassificationNCNN::ClassificationNCNN(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ClassificationNCNN)
{
    ui->setupUi(this);

    // qDebug()<<"OpenCV Version: "<<CV_VERSION;
    // qDebug()<<"OpenCV Major Version: "<CV_VERSION_MAJOR;
    // qDebug()<<"OpenCV Minor Version: "<<CV_VERSION_MINOR;

    //初始化索引到对应类别名称的映射 name = ['黛西','蒲公英','玫瑰','向日葵','郁金香']
    QList<QString> nameList = {"黛西","蒲公英","玫瑰","向日葵","郁金香"};
    for(int i = 0 ; i < nameList.size(); i++){
        this -> indexMapName[i] = nameList[i];
    }

    ui -> comboBox -> addItem("custom_model");
    ui -> comboBox -> addItem("mobilenet_v3_small");

    connect(ui -> comboBox,&QComboBox::currentIndexChanged,this,[=](){
        this -> model_name = ui -> comboBox -> itemText(ui -> comboBox -> currentIndex());
    });

    this -> mClasses = this -> readClassesFromFile(":/new/prefix1/resources/imageNet_classes.txt");

}

ClassificationNCNN::~ClassificationNCNN()
{
    delete ui;
}

QList<QString> ClassificationNCNN::readClassesFromFile(const QString &filePath) {
    QList<QString> mClasses;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open file for reading:" << filePath;
        return mClasses;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed(); // 读取一行并去除首尾空白
        if (!line.isEmpty()) {
            mClasses.append(line);
        }
    }

    file.close(); // 关闭文件
    return mClasses;
}

QImage ClassificationNCNN::cvMatToQImage(const cv::Mat &inMat) {
    switch (inMat.type()) {
    // 8-bit, 4 channel
    //如果是自己加入OpenCV的依赖库和头文件：static_cast<int>(inMat.step),//图像矩阵元素类型
    //否者如果使用simpleocv.h 则去掉static_cast<int>(inMat.step)
    case CV_8UC4: {
        QImage image(
            inMat.data,//图像数据
            inMat.cols,//图像宽度
            inMat.rows,//图像高度
            // static_cast<int>(inMat.step),
            QImage::Format_ARGB32//图像的像素格式
            );
        return image;
    }

        // 8-bit, 3 channel
    case CV_8UC3: {
        QImage image(inMat.data,
                     inMat.cols,
                     inMat.rows,
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
                     QImage::Format_Indexed8);
        image.setColorTable(sColorTable);
        return image;
    }
    default:
        break;
    }
    return QImage();
}

int ClassificationNCNN::detect_net(const cv::Mat& bgr, std::vector<float>& cls_scores){
    ncnn::Net squeezenet;

    squeezenet.opt.use_vulkan_compute = true;

    // the ncnn model https://github.com/nihui/ncnn-assets/tree/master/models
    if (squeezenet.load_param(this -> modelPath.toStdString().c_str()))
        exit(-1);
    if (squeezenet.load_model(QString(this -> modelPath.split(".")[0] + ".bin").toStdString().c_str()))
        exit(-1);

    ncnn::Mat in = ncnn::Mat::from_pixels_resize(bgr.data, ncnn::Mat::PIXEL_BGR, bgr.cols, bgr.rows, 224, 224);

    const float mean_vals[3] = {0.485f*255.f, 0.456f*255.f, 0.406f*255.f};
    const float norm_vals[3] = {1/0.229f/255.f, 1/0.224f/255.f, 1/0.225f/255.f};
    in.substract_mean_normalize(mean_vals, norm_vals);

    ncnn::Extractor ex = squeezenet.create_extractor();

    ex.input("input", in);

    ncnn::Mat out;
    ex.extract("predictions", out);

    cls_scores.resize(out.w);
    for (int j = 0; j < out.w; j++){
        cls_scores[j] = out[j];
    }

    return 0;
}

std::vector<float> ClassificationNCNN::softMax(std::vector<float> cls_scores){
    std::vector<float> probs;
    probs.resize(cls_scores.size());

    float maxLogit = -FLT_MAX; //得到系统的负无穷
    //遍历找到cls_scores中的最大值，避免溢出
    for(float logit : cls_scores){
        if(maxLogit < logit){
            maxLogit = logit;
        }
    }
    float sum = 0.0f;
    //根据softmax计算公式
    for(int i = 0 ; i < cls_scores.size(); i++){
        probs[i] = (float) exp(cls_scores[i] - maxLogit);
        sum += probs[i];
    }
    for(int i = 0; i < probs.size(); i++){
        probs[i] /= sum;
    }
    return probs;
}

int ClassificationNCNN::print_topk(const std::vector<float>& cls_scores, int topk){
    // partial sort topk with index
    int size = cls_scores.size();
    std::vector<std::pair<float, int> > vec;
    vec.resize(size);
    for (int i = 0; i < size; i++){
        vec[i] = std::make_pair(cls_scores[i], i);
    }

    std::partial_sort(vec.begin(), vec.begin() + topk, vec.end(),
                      std::greater<std::pair<float, int> >());

    // print topk and score
    for (int i = 0; i < topk; i++){
        float score = vec[i].first;
        int index = vec[i].second;
        fprintf(stderr, "%d = %f\n", index, score);
    }

    if(this -> model_name.compare("mobilenet_v3_small") == 0){
        //只显示最大概率的类别名称以及对应的概率
        ui -> label_result -> setText(QString("%1  %2%").arg(this -> mClasses[vec[0].second]).arg(QString::number(vec[0].first * 100,'f',2)));
    }else{
        //只显示最大概率的类别名称以及对应的概率
        ui -> label_result -> setText(QString("%1  %2%").arg(this -> indexMapName[vec[0].second]).arg(QString::number(vec[0].first * 100,'f',2)));
    }

    return 0;
}


void ClassificationNCNN::on_btn_open_file_clicked()
{
    this -> imgPath = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(this -> imgPath.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    qDebug()<<"image path: "<<this -> imgPath;

    this -> Image = cv::imread(this -> imgPath.toStdString(),cv::IMREAD_COLOR);
    if(this -> Image.empty()){
        QMessageBox::information(this,"提示","打开图像失败");
        return;
    }

    qDebug()<<"height = "<<this -> Image.rows<<" width = "<<this -> Image.cols;

    //将当前读取的图像缩放到和显示图像的布局大小相同
    QSize labSize = ui -> label_image -> size();

    QImage image(this -> imgPath);
    QImage scaled_img = image.scaled(labSize.rwidth(),labSize.rheight(),Qt::KeepAspectRatio);
    ui -> label_image -> setPixmap(QPixmap::fromImage(image));
}


void ClassificationNCNN::on_btn_open_model_clicked()
{
    QString model_path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(model_path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    this -> modelPath = model_path;
}


void ClassificationNCNN::on_btn_predict_clicked()
{
    //开始预测时间
    QDateTime curr = QDateTime::currentDateTime();
    qint64 startTime = curr.toMSecsSinceEpoch();
    //使用opencv读取图片
    cv::Mat m = this -> Image;
    if (m.empty()){
        qDebug()<<"The image path: "<<this -> imgPath;
        return;
    }

    std::vector<float> cls_scores;
    this -> detect_net(m, cls_scores);
    if(model_name.compare("mobilenet_v3_small") == 0){
        cls_scores = this -> softMax(cls_scores);
    }
    //打印得分前三的类别
    this -> print_topk(cls_scores, 3);

    //结束预测时间
    curr = QDateTime::currentDateTime();
    qint64 endTime = curr.toMSecsSinceEpoch();
    ui -> label_time -> setText(QString::number(endTime - startTime) + "ms");
}


void ClassificationNCNN::on_btn_close_clicked()
{
    this ->  close();
}

