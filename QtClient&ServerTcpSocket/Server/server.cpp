#include "server.h"
#include "ui_server.h"

#include <QMessageBox>
#include <QFileDialog>

Server::Server(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Server)
{
    ui->setupUi(this);

    this -> setFixedSize(QSize(564,332));
    this -> setWindowTitle("Server");

    this -> server = new QTcpServer(this);

    ui -> comboBox_transfer_type -> addItem("BroadCast");
    ui -> comboBox_transfer_type -> addItem("OnlyOne");
}

Server::~Server()
{
    delete ui;
}


QString Server::getLocalIp()
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

void Server::readSocket()
{
    QTcpSocket * socket = reinterpret_cast<QTcpSocket*>(sender());
    QByteArray DataBuffer;

    QDataStream socketStream(socket);
    socketStream.startTransaction();

    socketStream >> DataBuffer;

    //判断当前操作是否有效
    if(socketStream.commitTransaction() == false){
        return;
    }
    //解析来自客户端发送的文件信息
    QString headInfo = DataBuffer.mid(0,128);
    QString fileName = headInfo.split(",")[0].split(":")[1];
    QString fileExt = headInfo.split(":")[1];
    QString fileSize = headInfo.split(",")[1].split(":")[1];

    QString save_file_path = this -> fileDir + "/" + fileName;
    QFile file(save_file_path);
    if(file.open(QIODevice::WriteOnly)){
        file.write(DataBuffer);
        file.close();
    }
}

void Server::discardSocket()
{
    QTcpSocket * socket = reinterpret_cast<QTcpSocket*>(sender());
    int index = this -> client_list.indexOf(socket);
    if(index > -1){
        this -> client_list.remove(index);
    }
    ui -> comboBox -> clear();
    foreach(QTcpSocket * socket,this -> client_list){
        ui -> comboBox -> addItem(QString::number(socket -> socketDescriptor()));
    }
    socket -> deleteLater();
}

void Server::newConnection()
{
    while(this -> server -> hasPendingConnections()){
        this -> AddToSocketList(this -> server -> nextPendingConnection());
    }
}

void Server::AddToSocketList(QTcpSocket *socket)
{
    this -> client_list.append(socket);

    connect(socket,SIGNAL(readyRead()),this,SLOT(onreadyReady()));
    connect(socket,&QTcpSocket::disconnected,this,&Server::discardSocket);
    connect(socket,&QTcpSocket::stateChanged,this,&Server::onSocketStateChange);

    ui -> comboBox -> addItem(QString::number(socket -> socketDescriptor()));
    QString Client = "Client: " + QString::number(socket -> socketDescriptor()) + " Connected With The Server.";
    ui -> plainTextEdit -> appendPlainText(Client);
}

void Server::onreadyReady(){
    //sender() 是Qt中QObject类的一个成员函数，它返回最近发送信号给当前对象的对象的指针
    QTcpSocket * socket = reinterpret_cast<QTcpSocket*>(sender());
    QByteArray DataBuffer;

    QDataStream socketStream(socket);
    // socketStream.setVersion(QDataStream::Qt_6_4);

    socketStream.startTransaction();
    socketStream >> DataBuffer;

    //判断当前操作是否有效
    if(socketStream.commitTransaction() == false){
        return;
    }
    //解析来自客户端发送的文件信息
    QString headInfo = DataBuffer.mid(0,128);
    QString fileName = headInfo.split(",")[0].split(":")[1];
    QString fileExt = headInfo.split(".")[1];
    QString fileSize = headInfo.split(",")[1].split(":")[1];

    QString save_file_path = this -> fileDir + "/" + fileName;
    ui -> plainTextEdit -> appendPlainText("fileSize: " + fileSize);
    ui -> plainTextEdit -> appendPlainText("send file path: " + save_file_path);

    DataBuffer = DataBuffer.mid(128);
    QFile file(save_file_path);
    if(file.open(QIODevice::WriteOnly)){
        file.write(DataBuffer);
        file.close();
    }else{
        ui -> plainTextEdit -> appendPlainText("打开保存文件失败");
    }

    ui -> plainTextEdit -> appendPlainText("文件接收成功");
}

void Server::sendFile(QTcpSocket *socket, QString fileName)
{
    if(socket && socket -> isOpen()){
        QFile file(fileName);
        if(file.open(QIODevice::ReadOnly)){
            QFileInfo fileInfo(fileName);
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
            qDebug()<<"server: "<<byteFileData.size();
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

void Server::onSocketStateChange(QAbstractSocket::SocketState socketState){
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

void Server::on_btn_listen_clicked()
{
    QString localIP = this -> getLocalIp();
    qint64 port = 8080;

    ui -> plainTextEdit -> appendPlainText("IP: " + localIP);
    ui -> plainTextEdit -> appendPlainText("Port: " + QString::number(port));

    QHostAddress addr(localIP);
    this -> server -> listen(addr,port);
    ui -> plainTextEdit -> appendPlainText("服务端处于监听状态...");

    connect(this -> server,&QTcpServer::newConnection,this,&Server::newConnection);
}


void Server::on_btn_disconnect_clicked()
{
    // 在这里执行清理工作
    int count = ui -> comboBox -> count();
    for(int i = count - 1; i >= 0; i--){
        qDebug()<<"disconnect: "<<ui -> comboBox -> itemText(i);
        if(ui -> comboBox -> itemText(i) != "All Socket"){
            ui -> comboBox -> removeItem(i);
        }
    }
    foreach(QTcpSocket * socket,this -> client_list){
        socket -> disconnectFromHost();
        socket -> close();
    }
    ui -> plainTextEdit -> appendPlainText("Disconnected from the server");
}


void Server::on_btn_send_file_clicked()
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

    if(ui -> comboBox_transfer_type -> currentText() == "BroadCast"){
        foreach(QTcpSocket * socket,this -> client_list){
            this -> sendFile(socket,path);
        }
    }else if(ui -> comboBox_transfer_type -> currentText() == "OnlyOne"){
        foreach(QTcpSocket * socket,this -> client_list){
            if(ui -> comboBox -> currentText().toLongLong() == socket -> socketDescriptor()){
                this -> sendFile(socket,path);
            }
        }
    }
}


void Server::on_btn_close_clicked()
{
    this -> close();
}


void Server::on_btn_save_clicked()
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

