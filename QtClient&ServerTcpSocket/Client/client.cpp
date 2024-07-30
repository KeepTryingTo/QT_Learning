#include "client.h"
#include "ui_client.h"

#include <QMessageBox>

Client::Client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Client)
{
    ui->setupUi(this);

    this -> setFixedSize(QSize(588,305));
    this -> setWindowTitle("Client");

    this -> socket = new QTcpSocket(this);

    QString localIP = this -> getLocalIp();
    ui -> comboBox_IP -> addItem(localIP);

    connect(socket,SIGNAL(readyRead()),this,SLOT(readSocket()));
    connect(socket,&QTcpSocket::disconnected,this,&Client::discardSocket);
    connect(socket,&QTcpSocket::stateChanged,this,&Client::onSocketStateChange);
}

Client::~Client()
{
    delete ui;
}

QString Client::getLocalIp()
{
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

void Client::readSocket()
{
    QByteArray DataBuffer;

    QDataStream socketStream(this -> socket);
    // socketStream.setVersion(QDataStream::Qt_6_4);

    socketStream.startTransaction();
    socketStream >> DataBuffer;

    //判断当前操作是否有效
    if(socketStream.commitTransaction() == false){
        return;
    }
    //解析来自客户端发送的文件信息;返回一个新的 QByteArray 对象，包含从指定位置开始、指定长度的字节序列。
    QString headInfo = QString::fromUtf8(DataBuffer.mid(0,128));
    QString fileName = headInfo.split(",")[0].split(":")[1];
    QString fileExt = headInfo.split(".")[1];
    QString fileSize = headInfo.split(",")[1].split(":")[1];

    QString save_file_path = this -> fileDir + "/" + fileName;
    ui -> plainTextEdit -> appendPlainText("fileSize: " + fileSize);
    ui -> plainTextEdit -> appendPlainText("send file path: " + save_file_path);

    DataBuffer = DataBuffer.mid(128);
    // qDebug()<<"client: "<<DataBuffer.size();
    QFile file(save_file_path);
    if(file.open(QIODevice::WriteOnly)){
        file.write(DataBuffer);
        file.close();
    }else{
        ui -> plainTextEdit -> appendPlainText("打开保存文件失败");
    }

    ui -> plainTextEdit -> appendPlainText("文件接收成功");
}

void Client::discardSocket()
{
    this -> socket -> deleteLater();
}

void Client::sendFile(QTcpSocket *socket, QString fileName)
{
    if(this -> socket && this -> socket -> isOpen()){
        QFile file(fileName);
        if(file.open(QIODevice::ReadOnly)){
            QFileInfo fileInfo(file);
            ui -> plainTextEdit -> appendPlainText("fileName: " + fileInfo.fileName());
            ui -> plainTextEdit -> appendPlainText("fileSize: " + QString::number(fileInfo.size()));

            QDataStream socketStream(socket);
            // socketStream.setVersion(QDataStream::Qt_6_4);

            QByteArray header;
            QByteArray headInfo = "fileName:" + fileInfo.fileName().toUtf8() + ",fileSize:" + QString::number(fileInfo.size()).toUtf8();
            //headInfo: QString => QByteArray
            header.prepend(headInfo);
            header.resize(128);

            QByteArray byteFileData = file.readAll();
            byteFileData.prepend(header);


            socketStream<<byteFileData;

            ui -> plainTextEdit -> appendPlainText("文件发送成功");
        }else{
            // QMessageBox::information(this,tr("提示"),"文件打开失败");
            ui -> plainTextEdit -> appendPlainText("文件打开失败");
        }
    }else{
        ui -> plainTextEdit -> appendPlainText("socket is not invalid and not  open");
    }
}

void Client::onSocketStateChange(QAbstractSocket::SocketState socketState){
    switch (socketState) {
    case QAbstractSocket::UnconnectedState:
        ui -> label_state -> setText("UnconnectedState");
        break;
    case QAbstractSocket::HostLookupState:
        ui -> label_state -> setText("HostLookupState");
        break;
    case QAbstractSocket::ConnectingState:
        ui -> label_state -> setText("ConnectingState");
        break;
    case QAbstractSocket::ConnectedState:
        ui -> label_state -> setText("ConnectedState");
        break;
    case QAbstractSocket::BoundState:
        ui -> label_state -> setText("BoundState");
        break;
    case QAbstractSocket::ClosingState:
        ui -> label_state -> setText("ClosingState");
        break;
    case QAbstractSocket::ListeningState:
        ui -> label_state -> setText("ListeningState");
        break;
    default:
        ui -> label_state -> setText("其他未知状态...");
        break;
    }
}

void Client::on_btn_connect_clicked(){
    QString IP = ui -> comboBox_IP -> currentText();
    qint64 port = ui -> spinBox -> value();
    QHostAddress addr(IP);

    ui -> plainTextEdit -> appendPlainText("IP: " + IP);
    ui -> plainTextEdit -> appendPlainText("Port: " + QString::number(port));

    this -> socket -> connectToHost(addr,port);
    this -> socket -> open(QIODevice::ReadWrite);

    if(this -> socket -> isOpen()){
        ui -> plainTextEdit -> appendPlainText("Client connect successfully!");
    }else{
        ui -> plainTextEdit -> appendPlainText("Client not connect successfully!");
        return;
    }
    ui -> plainTextEdit -> appendPlainText("连接成功...");
}

void Client::on_btn_disconnect_clicked(){
    if(!this -> disconnect()){
        this -> disconnect();
        this -> close();
    }
    ui -> plainTextEdit -> appendPlainText("断开连接...");
}

void Client::on_btn_close_clicked(){
    this -> close();
}

void Client::on_btn_send_file_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,QString("打开文件"),".",tr("All Files(*.*);;MP3 Files(*.mp3);;MP4 Files(*.mp4)"));
    if(path.isEmpty()){
        QMessageBox::information(this,"提示","未选择文件");
        return;
    }
    QFileInfo fileinformation(path);
    QString fileName = fileinformation.fileName();
    ui -> plainTextEdit -> appendPlainText("send file dir: " + path);

    qDebug() <<"the path is: "<< path;

    this -> sendFile(this -> socket,path);

}


void Client::on_pushButton_clicked()
{
    //打开用于保存下载文件的文件夹;ShowDirsOnly只显示目录（文件夹）而不显示文件
    this -> fileDir = QFileDialog::getExistingDirectory(this,"选择保存路径",QString(),QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if(this -> fileDir.isEmpty()){
        QMessageBox::information(this,tr("提示"),"选择文件路径");
        return;
    }
    qDebug()<<"select dir: "<<this -> fileDir;
    ui -> plainTextEdit -> appendPlainText("save path dir: " + this -> fileDir);
}

