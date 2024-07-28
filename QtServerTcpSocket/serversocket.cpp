#include "serversocket.h"
#include "ui_serversocket.h"

#include <QFile>

ServerSocket::ServerSocket(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ServerSocket)
{
    ui->setupUi(this);

    initState();

    //添加样式
    QFile file;
    file.setFileName(":/new/prefix1/resources/css/qthttpcss.css");
    file.open(QIODevice::ReadOnly);

    QString strCss = file.readAll();
    this -> setStyleSheet(strCss);

    connect(this -> server,&QTcpServer::newConnection,this,&ServerSocket::onNewConnection);
}

ServerSocket::~ServerSocket()
{
    delete ui;
}

QString ServerSocket::getLocalIp()
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

void ServerSocket::initState()
{
    this -> server = new QTcpServer(this);
}

void ServerSocket::onNewConnection()
{
    while(this -> server -> hasPendingConnections()){
        this -> AddNewClientConnection(this -> server -> nextPendingConnection());
    }
}

void ServerSocket::AddNewClientConnection(QTcpSocket *socket)
{
    this -> sockets.append(socket);
    connect(socket,SIGNAL(readyRead()),this,SLOT(onreadyReady()));

    connect(socket, &QTcpSocket::errorOccurred, this, &ServerSocket::onError); //处理连接错误
    connect(socket,&QTcpSocket::stateChanged,this,&ServerSocket::onSocketStateChange);

    ui -> comboBox -> addItem(QString::number(socket -> socketDescriptor()));
    QString Client = "Client: " + QString::number(socket -> socketDescriptor()) + " Connected With The Server.";
    ui -> plainTextEdit -> appendPlainText(Client);
}

void ServerSocket::onreadyReady(){
    QTcpSocket * socket = reinterpret_cast<QTcpSocket*>(sender());
    QByteArray array = socket -> readAll();
    QString info = "[Client: " + QString::number(socket -> socketDescriptor()) + "]" + QString::fromStdString(array.toStdString());
    ui -> plainTextEdit -> appendPlainText(info);
}


void ServerSocket::onDisconnected() {
    // 在这里执行清理工作
    int count = ui -> comboBox -> count();
    for(int i = count - 1; i >= 0; i--){
        qDebug()<<"disconnect: "<<ui -> comboBox -> itemText(i);
        if(ui -> comboBox -> itemText(i) != "All Socket"){
            ui -> comboBox -> removeItem(i);
        }
    }
    foreach(QTcpSocket * socket,this -> sockets){
        socket -> disconnectFromHost();
        socket -> close();
    }
    ui -> plainTextEdit -> appendPlainText("Disconnected from the server");
}

void ServerSocket::onError(QAbstractSocket::SocketError socketError) {
    if (socketError == QAbstractSocket::RemoteHostClosedError) {
        ui -> plainTextEdit -> appendPlainText("The remote host closed the connection.");
    }
}

void ServerSocket::onSocketStateChange(QAbstractSocket::SocketState socketState){
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


void ServerSocket::on_btn_close_clicked()
{
    this -> server -> disconnect();
    this -> server -> close();
    this -> close();
}


void ServerSocket::on_btn_clear_clicked()
{
    ui -> plainTextEdit -> clear();
}


void ServerSocket::on_btn_disconnect_clicked()
{
    this -> onDisconnected();
}


void ServerSocket::on_btn_listen_clicked()
{
    if(!this -> server){
        this -> server = new QTcpServer(this);
    }
    QString localIP = this -> getLocalIp();
    QHostAddress addr(localIP);
    this -> server -> listen(addr,8080);
}


void ServerSocket::on_btn_send_clicked()
{
    QString info = ui -> lineEdit -> text();
    if(ui -> comboBox -> currentText() == "All Socket"){
        foreach(QTcpSocket * socket,this -> sockets){
            socket -> write(info.toStdString().c_str());
        }
    }else{
        foreach(QTcpSocket * socket,this -> sockets){
            if(socket -> socketDescriptor() == ui -> comboBox -> currentText().toLongLong()){
                socket -> write(info.toStdString().c_str());
            }
        }
    }
    ui -> lineEdit -> clear();
    ui -> plainTextEdit -> appendPlainText("[Server: ]" + info);
}

