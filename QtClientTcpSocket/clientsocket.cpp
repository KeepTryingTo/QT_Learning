#include "clientsocket.h"
#include "ui_clientsocket.h"

#include <QMessageBox>
#include <QDebug>
#include <QSize>
#include <QHostInfo>
#include <QFile>

ClientSocket::ClientSocket(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ClientSocket)
{
    ui->setupUi(this);

    //添加样式
    QFile file;
    file.setFileName(":/new/prefix1/resources/css/qthttpcss.css");
    file.open(QIODevice::ReadOnly);

    QString strCss = file.readAll();
    this -> setStyleSheet(strCss);

    this -> initSocket();

    this -> setWindowTitle("Client TcpSocket");
    this -> setMaximumSize(QSize(403,255));

    connect(socket, &QTcpSocket::disconnected, this, &ClientSocket::onDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &ClientSocket::onError); //处理连接错误
    connect(socket,SIGNAL(readyRead()),this,SLOT(onreadyReady()));
    connect(socket,&QTcpSocket::stateChanged,this,&ClientSocket::onSocketStateChange);
}

ClientSocket::~ClientSocket(){
    delete ui;
}


QString ClientSocket::getLocalIp()
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

void ClientSocket::initSocket(){
    this -> socket = new QTcpSocket(this);
}

void ClientSocket::on_btn_close_clicked(){
    this -> socket -> disconnectFromHost();
    this -> close();
}

void ClientSocket::on_btn_send_clicked(){
    if(this -> socket){
        if(this -> socket -> isOpen()){
            QString writeData = ui -> lineEdit -> text();
            this -> socket -> write(writeData.toStdString().c_str());
            ui -> lineEdit -> clear();
            ui -> plainTextEdit -> appendPlainText("[Client]: " + writeData);
        }
    }
    //否则将会触发错误信号，输出当前错误信息
}

void ClientSocket::onreadyReady(){
    // qDebug()<<"ready: "<<this -> socket -> readAll();
    // if(this -> socket -> canReadLine()){
        // while(this -> socket -> canReadLine()){
            QByteArray array = this -> socket -> readAll();
            ui -> plainTextEdit -> appendPlainText("[Server]: " + QString::fromStdString(array.toStdString()));
        // }
    // }
}

void ClientSocket::on_btn_disconnect_clicked(){
    this -> socket -> disconnectFromHost();
}

void ClientSocket::on_btn_connect_clicked(){
    if(!this -> socket){
        this -> socket = new QTcpSocket();
    }
    //得到本机IPv4地址
    QString localIP = this -> getLocalIp();
    QHostAddress addr(localIP);
    ui -> plainTextEdit -> appendPlainText("IP: " + localIP + "  Port: " + QString::number(10000));

    this -> socket -> connectToHost(addr,10000);
    this -> socket -> open(QIODevice::ReadWrite);

    if(this -> socket -> isOpen()){
        ui -> plainTextEdit -> appendPlainText("Client connect successfully!");
    }else{
        ui -> plainTextEdit -> appendPlainText("Client not connect successfully!");
    }
}

void ClientSocket::onDisconnected() {
    // 在这里执行清理工作
    this -> socket -> close();
    ui -> plainTextEdit -> appendPlainText("Disconnected from the server");
}

void ClientSocket::onError(QAbstractSocket::SocketError socketError) {
    ui -> plainTextEdit -> appendPlainText("Socket error: " + socket->errorString());
    if (socketError == QAbstractSocket::RemoteHostClosedError) {
        ui -> plainTextEdit -> appendPlainText("The remote host closed the connection.");
    }
}

void ClientSocket::onSocketStateChange(QAbstractSocket::SocketState socketState){
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

void ClientSocket::on_btn_clear_clicked(){
    ui -> plainTextEdit -> clear();
}

