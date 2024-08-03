#include "client.h"
#include "ui_client.h"

Client::Client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Client)
{
    ui->setupUi(this);

    this -> socket = new QTcpSocket(this);

    QString ip = this -> getLocalIp();
    qint64 port = 8080;

    QHostAddress addr(ip);
    this -> socket -> connectToHost(addr,port);

    connect(socket,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
    connect(socket, &QTcpSocket::errorOccurred, this, &Client::onError); //处理连接错误
    connect(socket,&QTcpSocket::stateChanged,this,&Client::onSocketStateChange);
}

Client::~Client(){
    delete ui;
}

QString Client::getLocalIp(){
    QString hostName = QHostInfo::localHostName();
    QHostInfo hostInfo = QHostInfo::fromName(hostName);

    qDebug()<<"本机名称: " + hostName;
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

void Client::on_btn_send_clicked(){
    QString msg = ui -> textEdit -> toPlainText();
    this -> socket -> write(msg.toUtf8().constData());
    ui -> textEdit -> clear();
}

void Client::onError(QAbstractSocket::SocketError socketError) {
    if (socketError == QAbstractSocket::RemoteHostClosedError) {
        qDebug()<<"The remote host closed the connection.";
    }
}

void Client::onSocketStateChange(QAbstractSocket::SocketState socketState){
    switch (socketState) {
    case QAbstractSocket::UnconnectedState:
        qDebug()<<"UnconnectedState";
        break;
    case QAbstractSocket::HostLookupState:
        qDebug()<<"HostLookupState";
        break;
    case QAbstractSocket::ConnectingState:
        qDebug()<<"ConnectingState";
        break;
    case QAbstractSocket::ConnectedState:
        qDebug()<<"ConnectedState";
        break;
    case QAbstractSocket::BoundState:
        qDebug()<<"BoundState";
        break;
    case QAbstractSocket::ClosingState:
        qDebug()<<"ClosingState";
        break;
    case QAbstractSocket::ListeningState:
        qDebug()<<"ListeningState";
        break;
    default:
        qDebug()<<"其他未知状态...";
        break;
    }
}

void Client::addTableWidgetItem(){
    int number = 0;
    //更新当前好友和在线列表
    int row = 0;
    ui -> tableWidget -> clear();//首先把之前的用户清单清除之后再重新更新用户列表
    //设置表的行和列大小
    ui -> tableWidget -> setColumnCount(2);
    ui -> tableWidget -> setRowCount(this -> map.count());
    QStringList headerLabel;
    headerLabel << "在线好友"<<"上线时间";
    ui -> tableWidget -> setHorizontalHeaderLabels(headerLabel);

    for(const auto &key : map.keys()){
        if(map.value(key).compare("0") !=0 ){
            QTableWidgetItem * keyItem = new QTableWidgetItem(QString::number(key));
            QTableWidgetItem * valueItem = new QTableWidgetItem(map.value(key));

            ui -> tableWidget -> setItem(row,0,keyItem);
            ui -> tableWidget -> setItem(row,1,valueItem);
            row ++;
            number ++;
        }
    }
    ui -> label_number -> setText(QString::number(number));
}

void Client::onReadyRead(){
    QByteArray array = this -> socket -> readAll();
    // qDebug()<<"recv: "<<QString(array);
    if(QString(array).contains("#map#")){
        QString mapData = QString(array).mid(5);
        qDebug()<<"mapData: "<<mapData;
        int key = mapData.split(",")[0].toInt();
        QString value = mapData.split(",")[1];
        this -> map[key] = value;

        this -> addTableWidgetItem();
    }else{
        this -> addTableWidgetItem();
        ui -> plainTextEdit -> appendPlainText(QString(array));
    }
}

void Client::on_btn_close_clicked(){
    this -> socket -> disconnectFromHost();
    this -> socket -> close();
    this -> socket -> deleteLater();
    this -> close();
}

