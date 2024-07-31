#include "ftpclient.h"
#include "ui_ftpclient.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QHostAddress>
#include <QHostInfo>

FTPClient::FTPClient(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::FTPClient)
{
    ui->setupUi(this);

    //refer to: https://blog.csdn.net/qq_44540071/article/details/140325358
    //refer to: https://blog.csdn.net/2301_78216479/article/details/138161487

    QString localIp = this -> getLocalIp();
    ui -> lineEdit_ip -> setText(localIp);

    this -> networkAccessManager = new QNetworkAccessManager();

    //打印当前QNetworkAccessManager所支持的协议
    QStringList schemeList = this -> networkAccessManager -> supportedSchemes();
    ui -> plainTextEdit -> appendPlainText("支持的协议: ");
    foreach(const QString & scheme,schemeList){
        ui -> plainTextEdit -> appendPlainText(scheme);
    }//从打印结果可以看到，既然没有ftp协议

    //初始化传输类型，也就是上传文件还是下载文件
    ui -> comboBox -> addItem("Up");
    ui -> comboBox -> addItem("Down");
}

FTPClient::~FTPClient(){
    delete ui;
    this -> reply -> deleteLater();
    this -> networkAccessManager -> deleteLater();
    this -> file -> deleteLater();
}

QString FTPClient::getLocalIp(){
    QString hostName = QHostInfo::localHostName();
    QHostInfo hostInfo = QHostInfo::fromName(hostName);

    ui -> plainTextEdit -> appendPlainText("本机名称: " + hostName);
    QString localIP;

    QList<QHostAddress> list = hostInfo.addresses();
    if(list.empty())return "null QString IP";

    foreach(QHostAddress addr,list){
        if(addr.protocol() == QAbstractSocket::IPv4Protocol){
            localIP = addr.toString();
            break;
        }
    }
    return localIP;
}

QUrl FTPClient::applySetting(QString fileName){
    //协议，地址，端口，用户名和密码的配置；trimmed去掉字符串的首尾空格
    QString userName = ui -> lineEdit_user -> text().trimmed();
    QString password = ui -> lineEdit_pwd -> text().trimmed();
    QString ip = ui -> lineEdit_ip -> text().trimmed();//默认IP为本机IP地址，也就是使用本机的FTP服务器
    int port = ui -> spinBox -> value();
    //书写格式：协议://IP地址:Port端口/
    QString url = QString("ftp://%1:%2").arg(ip).arg(port) + "/" + fileName;

    QUrl ftpUrl(url);
    ui -> plainTextEdit -> appendPlainText("FTP服务器路径： " + url);
    ftpUrl.setUserName(userName);
    ftpUrl.setPassword(password);
    // ftpUrl.setPort(port);
    // ftpUrl.setScheme("ftp");
    return ftpUrl;
}

void FTPClient::uploadFile(QString path)
{
    QFileInfo fileinformation(path);
    QString fileName = fileinformation.fileName();
    qDebug() <<"the path is: "<< path;
    ui -> plainTextEdit -> appendPlainText("upload file path: " + path);
    //打开文件并上传文件过程
    this -> file = new QFile(path);
    QUrl ftpUrl = this -> applySetting(fileName);

    if(this -> file->open(QIODevice::ReadOnly)){
        QNetworkRequest request(ftpUrl);
        QByteArray readArray = this -> file -> readAll();
        this -> reply = networkAccessManager -> put(request,readArray);

        connect(this -> reply,&QNetworkReply::uploadProgress,this,&FTPClient::onUploadProgress);
        connect(this -> networkAccessManager,&QNetworkAccessManager::finished,this,&FTPClient::onFinished);
        connect(this -> reply,&QNetworkReply::errorOccurred,this,&FTPClient::onErrorOccurred);

        if(reply -> error() == QNetworkReply::NoError){
            ui -> plainTextEdit -> appendPlainText("请求成功");
        }else{
            QMessageBox::information(this,"提示","请求失败",QMessageBox::Ok);
            return;
        }
    }
}

void FTPClient::downloadFile(QString path)
{
    //由于这是FTP服务器下载文件，这里得到的路径path是服务器要下载的文件路径
    QFileInfo fileinformation(path);
    QString fileName = fileinformation.fileName();
    qDebug() <<"the path is: "<< path;
    ui -> plainTextEdit -> appendPlainText("download file path: " + path);

    QString serverFilePath = path.mid(7);//从选择的路径字符串，字符位置7开始，后面的作为从服务器下载的文件路径
    QUrl ftpUrl = this -> applySetting(serverFilePath);
    QNetworkRequest request(ftpUrl);
    this -> reply = this -> networkAccessManager -> get(request);

    //将从FTP服务器下载的文件保存到默认路径
    this -> file = new QFile(QCoreApplication::applicationDirPath() + "/" + fileName);
    ui -> plainTextEdit -> appendPlainText("save file path: " + QCoreApplication::applicationDirPath() + "/" + fileName);

    if(file -> open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Text)){
        connect(this -> reply,&QNetworkReply::downloadProgress,this,&FTPClient::onDownloadProgress);
        connect(this -> reply,&QNetworkReply::readyRead,this,&FTPClient::onReadReady);
        connect(this -> reply,&QNetworkReply::finished,this,[=](){
            if(this -> reply -> error()){
                ui -> plainTextEdit -> appendPlainText("请求失败");
            }else{
                this -> file -> close();
                ui -> plainTextEdit -> appendPlainText("下载文件成功");
            }
        });
    }else{
        QMessageBox::information(this,tr("提示"),"文件打开失败");
        return;
    }
}


void FTPClient::on_btn_open_clicked(){
    //选择上传文件过程
    QString path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return ;
    }

    //根据当前选择的是上传文件还是下载文件来执行程序
    if(ui -> comboBox -> currentIndex() == 0){
        this -> uploadFile(path);
    }else{
        this -> downloadFile(path);
    }

}

void FTPClient::on_btn_close_clicked(){
    this -> close();
}

void FTPClient::onUploadProgress(qint64 byteSent, qint64 totalBytes){
    ui -> progressBar -> setMaximum(totalBytes / 1024);
    ui -> progressBar -> setValue(byteSent / 1024);
    qDebug()<<"byteSent: "<<byteSent<<"  totalBytes: "<<totalBytes;
}

void FTPClient::onFinished(QNetworkReply *){
    this -> file -> close();
    if(reply -> error() == QNetworkReply::NoError){
        ui -> plainTextEdit -> appendPlainText("上传文件成功");
    }else{
        ui -> plainTextEdit -> appendPlainText("上传文件失败");
    }
}

void FTPClient::onErrorOccurred(QNetworkReply::NetworkError error)
{
    //注意这里可能出现错误：协议未知错误ProtocolUnknownError
    ui -> plainTextEdit -> appendPlainText("Reply Error: " + QString::number(error));
}

void FTPClient::onDownloadProgress(qint64 bytesRecv, qint64 totalBytes)
{
    ui -> progressBar -> setMaximum(totalBytes / 1024);
    ui -> progressBar -> setValue(bytesRecv / 1024);
    qDebug()<<"bytesRecv: "<<bytesRecv<<"  totalBytes: "<<totalBytes;
}

void FTPClient::onReadReady()
{
    this -> file -> write(this -> reply -> readAll());
}

