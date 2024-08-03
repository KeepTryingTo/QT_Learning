#include "server.h"
#include "ui_server.h"

#include <QTime>

Server::Server(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Server)
{
    ui->setupUi(this);

    this -> info = "";
    this -> initState();
    connect(this -> server,&QTcpServer::newConnection,this,&Server::onNewConnection);
}

Server::~Server()
{
    this -> server -> disconnect();
    this -> server -> close();
    this -> server -> deleteLater();
    delete ui;
}

QString Server::getLocalIp()
{
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

void Server::initState()
{
    this -> server = new QTcpServer(this);

    QString localIP = this -> getLocalIp();
    ui -> lineEdit -> setText(localIP);
}

void Server::onNewConnection()
{
    while(this -> server -> hasPendingConnections()){
        this -> AddNewClientConnection(this -> server -> nextPendingConnection());
    }
}
//用于睡眠的函数
void Server::onSleep(unsigned int milliseconds)
{
    //currnentTime 返回当前时间 用当前时间加上我们要延时的时间msec得到一个新的时刻
    QTime reachTime = QTime::currentTime().addMSecs(milliseconds);
    //用while循环不断比对当前时间与我们设定的时间
    while(QTime::currentTime() < reachTime){
        //如果当前的系统时间尚未达到我们设定的时刻，就让Qt的应用程序类执行默认的处理，
        //以使程序仍处于响应状态。一旦到达了我们设定的时刻，就跳出该循环，继续执行后面的语句。
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
}

void Server::AddNewClientConnection(QTcpSocket *socket)
{
    this -> sockets.append(socket);
    connect(socket,SIGNAL(readyRead()),this,SLOT(onreadyReady()));
    // connect(socket,&QTcpSocket::disconnected,this,&Server::onDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &Server::onError); //处理连接错误
    connect(socket,&QTcpSocket::stateChanged,this,&Server::onSocketStateChange);

    //如果当前有新的客户端和服务端建立了连接就向群里面发送一个消息，表示当前某个客户端好友上线了
    QString Client = "Client: " + QString::number(socket -> socketDescriptor()) + " 上线 " + QTime::currentTime().toString();
    info = Client;
    this -> send_to_socket();
    qDebug()<<Client;
    //保存当前客户端编号ID以及对应的时间
    this -> map[socket -> socketDescriptor()] = QTime::currentTime().toString();
    //首先睡眠100毫秒之后在发送当前和服务端建立的连接
    this -> onSleep(1000);
    this -> sendMapData();
}

void Server::sendMapData(){
    foreach(QTcpSocket * socket,this -> sockets){
        //将当前的关键词key以及对应的上线时间一起发送过去
        for(const auto &key : map.keys()){
            //发送正文消息之前首先发送一个标识，标识我后面发送的消息为map数据
            socket -> write(QString("#map#%1,%2").arg(key).arg(map.value(key)).toStdString().c_str());
            this -> onSleep(50);
        }
    }
}

void Server::onError(QAbstractSocket::SocketError socketError) {
    if (socketError == QAbstractSocket::RemoteHostClosedError) {
        qDebug()<<"The remote host closed the connection.";
    }
}

void Server::onSocketStateChange(QAbstractSocket::SocketState socketState){
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
    //用于输出当前还有哪些客户端和服务端是相连接的；如果是未连接状态，则为-1；否则为对应的描述器编号
    QList<int>keys = this -> map.keys();
    foreach(int key,keys){
        if(!this -> isContains(key)){
            this -> map[key] = "0";//如果和服务端已经断开了连接，则相应的键值key所对应的value设置为0
        }
    }
    //如果有客户端和服务端断开了连接的话，也要发一遍map数据，以便于客户端那边上线用户列表的数据更新
    this -> sendMapData();
}

bool Server::isContains(int id){
    foreach(QTcpSocket * socket,this -> sockets){
        if(id == socket -> socketDescriptor()){
            return true;
        }
    }
    return false;
}

void Server::onreadyReady(){
    QTcpSocket * socket = reinterpret_cast<QTcpSocket*>(sender());
    QByteArray array = socket -> readAll();
    this -> info = "[Client: " + QString::number(socket -> socketDescriptor()) + "]" + QString::fromStdString(array.toStdString());

    //如果当前服务器已经和客户端建立了连接并且有客户端向服务端发送消息,服务端就将消息以广播的额方式发送给所有的客户端，模拟群聊
    if(this -> sockets.length() > 0 && !this -> info.isEmpty()){
        this -> send_to_socket();
    }
}

void Server::onDisconnected() {
    // 在这里执行清理工作
    foreach(QTcpSocket * socket,this -> sockets){
        socket -> disconnectFromHost();
        socket -> close();
    }

}

void Server::server_listen()
{
    if(!this -> server){
        this -> server = new QTcpServer(this);
    }
    QString localIP = ui -> lineEdit -> text();
    qint64 port = ui -> spinBox -> value();
    QHostAddress addr(localIP);
    this -> server -> listen(addr,port);
}

void Server::send_to_socket()
{
    foreach(QTcpSocket * socket,this -> sockets){
        socket -> write(info.toStdString().c_str());
    }
}

void Server::on_pushButton_clicked()
{
    this -> server_listen();
    ui -> pushButton -> setEnabled(false);
}

