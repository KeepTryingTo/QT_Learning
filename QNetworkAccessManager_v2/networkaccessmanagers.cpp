#include "networkaccessmanagers.h"
#include "ui_networkaccessmanagers.h"


NetworkAccessManagers::NetworkAccessManagers(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NetworkAccessManagers)
{
    ui->setupUi(this);

    //refer to: https://github.com/13506525537/HTTPSDownloadClient

    ui -> progressBar -> setValue(0);
    ui -> progressBar -> setMinimum(0);

    //添加样式
    QFile file;
    file.setFileName(":/new/prefix1/resources/css/qthttpcss.css");
    file.open(QIODevice::ReadOnly);

    QString strCss = file.readAll();
    this -> setStyleSheet(strCss);

    this -> setWindowTitle("QtNetworkAccessManager&Download");
    this -> setFixedSize(QSize(464,254));

    this -> Nmanager = new QNetworkAccessManager(this);

    //选择下载的文件类型
    ui -> comboBox -> addItem("JPG");
    ui -> comboBox -> addItem("PNG");
    ui -> comboBox -> addItem("ZIP");
    ui -> comboBox -> addItem("MP3");
    ui -> comboBox -> addItem("MP4");
}

NetworkAccessManagers::~NetworkAccessManagers(){
    delete ui;
}

void NetworkAccessManagers::on_btn_close_clicked(){
    this -> close();
}

void NetworkAccessManagers::on_btn_save_clicked(){
    //打开用于保存下载文件的文件夹;ShowDirsOnly只显示目录（文件夹）而不显示文件
    this -> fileDir = QFileDialog::getExistingDirectory(this,"选择保存路径",QString(),QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if(this -> fileDir.isEmpty()){
        QMessageBox::information(this,tr("提示"),"选择文件路径");
        return;
    }
    qDebug()<<"select dir: "<<this -> fileDir;
    ui -> plainTextEdit -> appendPlainText("save path: " + this -> fileDir);
}


void NetworkAccessManagers::on_btn_download_clicked(){

    QString url = ui -> lineEdit -> text().trimmed(); //除去给定地址存在的空格
    ui -> lineEdit -> clear();

    QUrl APIurl = QUrl::fromUserInput(url);
    if(!APIurl.isValid()){
        QMessageBox::information(this,"提示","地址无效",QMessageBox::Ok);
    }

    int index = ui -> comboBox -> currentIndex();//获得当前下载的文件类型索引
    QString fileName = this -> fileDir + "/" + APIurl.fileName() + "." + ui -> comboBox -> itemText(index).toLower();
    ui -> plainTextEdit -> appendPlainText("fileName: " + fileName);

    fp = new QFile(fileName);
    //如果之前有相同的文件存在，则删除之前的文件
    if(fp->exists()){
        fp -> remove();
    }
    if(fp -> open(QIODevice::WriteOnly)){
        QNetworkRequest request(APIurl);

        this -> reply = this -> Nmanager -> get(request);
        // this -> loop = new QEventLoop(this);//循环事件，直到处理完成
        connect(this -> reply,&QNetworkReply::downloadProgress,this,&NetworkAccessManagers::onDownloadProgress);
        connect(this -> reply,&QNetworkReply::finished,this,&NetworkAccessManagers::onFinished);
        connect(this -> reply,&QNetworkReply::readyRead,this,&NetworkAccessManagers::onReadyRead);
        // this -> loop -> exec();

        if(this -> reply -> error() == QNetworkReply::NoError){
            ui -> btn_download -> setEnabled(false);
        }else{
            QMessageBox::information(this,"提示","请求失败",QMessageBox::Ok);
            return;
        }
    }else{
        QMessageBox::information(this,"提示","打开文件失败",QMessageBox::Ok);
        return;
    }
}

void NetworkAccessManagers::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal){
    ui -> progressBar -> setMaximum(bytesTotal / 1024);
    ui -> progressBar -> setValue(bytesReceived / 1024);
    qDebug()<<"current len: "<<bytesReceived;
}

void NetworkAccessManagers::onFinished(){
    ui -> btn_download -> setEnabled(true);
    this -> fp -> close();
    this -> reply -> deleteLater();
    ui -> plainTextEdit -> appendPlainText("下载完成");

    // this -> loop -> quit();
}

void NetworkAccessManagers::onReadyRead(){
    this -> fp -> write(this -> reply -> readAll());
}

