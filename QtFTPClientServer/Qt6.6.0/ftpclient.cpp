#include "ftpclient.h"
#include "ui_ftpclient.h"

#include <QFile>

FTPClient::FTPClient(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::FTPClient)
{
    ui->setupUi(this);

    QString localIp = this -> getLocalIp();
    ui -> lineEdit_ip -> setText(localIp);

    //初始化传输类型，也就是上传文件还是下载文件
    ui -> comboBox -> addItem("Up");
    ui -> comboBox -> addItem("Down");
}

FTPClient::~FTPClient()
{
    delete ui;
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

QString FTPClient::applySetting(QString fileName){
    //协议，地址，端口，用户名和密码的配置；trimmed去掉字符串的首尾空格
    QString userName = ui -> lineEdit_user -> text().trimmed();
    QString password = ui -> lineEdit_pwd -> text().trimmed();
    QString ip = ui -> lineEdit_ip -> text().trimmed();//默认IP为本机IP地址，也就是使用本机的FTP服务器
    int port = ui -> spinBox -> value();
    //书写格式：协议://用户名：密码@IP地址:Port端口/文件路径
    QString url;
    if(this -> isUpload){
        url = QString("ftp://%1:%2@%3:%4").arg(userName).arg(password).arg(ip).arg(port) + "/" + fileName;
    }else{
        url = QString("ftp://%1:%2@%3:%4").arg(userName).arg(password).arg(ip).arg(port) + "/" + fileName;
    }
    return url;
}

void FTPClient::uploadFile(QString path)
{
    QFileInfo fileinformation(path);
    QString fileName = fileinformation.fileName();
    qDebug() <<"the path is: "<< path;
    ui -> plainTextEdit -> appendPlainText("upload file path: " + path);
    //打开文件并上传文件过程
    this -> file = new QFile(path);

    QString url = this -> applySetting(fileName);
    QString userName = ui -> lineEdit_user -> text().trimmed();
    QString passWord = ui -> lineEdit_pwd -> text().trimmed();

    //将本地path文件上传到指定FTP服务器URL
    bool success = ftpUpload_and_download(url, userName, passWord, path);
    if (success) {
        qDebug() << "FTP Upload succeeded";
        ui -> plainTextEdit -> appendPlainText("上传文件成功");
    } else {
        qDebug() << "FTP Upload failed";
        ui -> plainTextEdit -> appendPlainText("上传文件失败");
    }
}

void FTPClient::downloadFile(QString path)
{
    //由于这是FTP服务器下载文件，这里得到的路径path是服务器要下载的文件路径
    QFileInfo fileinformation(path);
    QString fileName = fileinformation.fileName();
    qDebug() <<"download file path is: "<< path;
    ui -> plainTextEdit -> appendPlainText("download file path: " + path);

    QString serverFilePath = path.mid(7);//从选择的路径字符串，字符位置7开始，后面的作为从服务器下载的文件路径

    //将从FTP服务器下载的文件保存到默认路径
    QString save_path = QCoreApplication::applicationDirPath() + "/" + fileName;
    ui -> plainTextEdit -> appendPlainText("save file path: " + QCoreApplication::applicationDirPath() + "/" + fileName);

    QString url = this -> applySetting(serverFilePath);
    QString userName = ui -> lineEdit_user -> text().trimmed();
    QString passWord = ui -> lineEdit_pwd -> text().trimmed();

    //将指定FTP服务器URL上的文件下载并保存到本地路径save_path
    bool success = ftpUpload_and_download(url, userName, passWord, save_path);
    if (success) {
        qDebug() << "FTP Download Succeeded";
        ui -> plainTextEdit -> appendPlainText("下载文件成功");
    } else {
        qDebug() << "FTP Download Failed";
        ui -> plainTextEdit -> appendPlainText("下载文件失败");
    }
}

// 回调函数，用于接收数据
// size_t FTPClient::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
//     ((QByteArray*)userp)->append((char*)contents, size * nmemb);
//     ui -> progressBar -> setValue((size * nmemb) / 1024);
//     return size * nmemb;
// }
size_t FTPClient::WriteCallback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    QFile *file = static_cast<QFile*>(userdata);
    // ui -> progressBar -> setValue((size * nmemb) / 1024);
    return file->write(static_cast<char*>(ptr), size * nmemb);
}
//回调函数，用于上传数据
size_t FTPClient::ReadCallback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    //userdata读取数据的文件
    QFile *file = static_cast<QFile*>(userdata);
    // ui -> progressBar -> setValue((size * nmemb) / 1024);
    //ptr表示保存读取的数据
    return file->read(static_cast<char*>(ptr), size * nmemb);
}

// FTP上传函数
bool FTPClient::ftpUpload_and_download(const QString &url, const QString &username, const QString &password, const QString &localFilepath) {
    CURL *curl;
    CURLcode res;

    QFile *file = new QFile(localFilepath);
    if (this -> isUpload && !file -> open(QIODevice::ReadOnly)) {
        ui -> plainTextEdit -> appendPlainText("Failed to open file for reading:" + localFilepath);
        return false;
    }else if(!this -> isUpload && !file -> open(QIODevice::WriteOnly)){
        ui -> plainTextEdit -> appendPlainText("Failed to open file for writing:");
        return false;
    }

    qDebug()<<"url: "<<url;

    // this -> fileSize = file -> size();
    // ui -> progressBar -> setMaximum(this -> fileSize / 1024);
    QByteArray data;

    // struct curl_slist *headerlist = NULL;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        // 设置目标URL
        res = curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
        if(res != CURLE_OK)ui -> plainTextEdit -> appendPlainText("设置目标失败");

        // 设置用户名和密码
        res = curl_easy_setopt(curl, CURLOPT_USERNAME, username.toStdString().c_str());
        if(res != CURLE_OK)ui -> plainTextEdit -> appendPlainText("设置用户名失败");
        res = curl_easy_setopt(curl, CURLOPT_PASSWORD, password.toStdString().c_str());
        if(res != CURLE_OK)ui -> plainTextEdit -> appendPlainText("设置密码失败");

        if(this -> isUpload){
            // 设置上传操作
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        }

        // 设置使用active FTP协议
        res = curl_easy_setopt(curl, CURLOPT_FTPPORT, "-");
        if(res != CURLE_OK)ui -> plainTextEdit -> appendPlainText("设置协议失败");

        if(this -> isUpload){
            // 设置回调函数，用于读取文件数据
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, this -> ReadCallback);
            // 设置用户自定义数据
            res = curl_easy_setopt(curl, CURLOPT_READDATA, file);
            if(res != CURLE_OK)ui -> plainTextEdit -> appendPlainText("读取数据失败");
        }else{
            // 设置回调函数，用于读取文件数据
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, this -> WriteCallback);
            // 设置用户自定义数据
            res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
            if(res != CURLE_OK)ui -> plainTextEdit -> appendPlainText("写入数据失败");
        }

        // 根据本地上传到FTP服务器上的文件，设置文件大小
        curl_easy_setopt(curl, CURLOPT_INFILESIZE, (curl_off_t)file->size());

        // 执行FTP上传
        res = curl_easy_perform(curl);

        // 检查错误
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // 清理
        curl_easy_cleanup(curl);
        file->close();
        delete file;
        //释放由curl_global_init函数初始化时分配的全局资源,防止内存泄漏以及保证线程安全
        curl_global_cleanup();
        return (res == CURLE_OK);
    }
    ui -> plainTextEdit -> appendPlainText("CURL创建句柄失败");
    delete file;
    return false;
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
        this -> isUpload = false;
        this -> downloadFile(path);
    }
}

void FTPClient::on_btn_close_clicked(){
    this -> close();
}


