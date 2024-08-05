#include "udpgroupchat.h"
#include "ui_udpgroupchat.h"

UdpGroupChat::UdpGroupChat(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::UdpGroupChat)
{
    ui->setupUi(this);

    this -> udpSocket = new QUdpSocket(this);
    this -> udpBind();
    connect(this -> udpSocket,&QUdpSocket::readyRead,this,&UdpGroupChat::onReadDataGram);

    //刚开始打开的客户端上线，首先会向其他客户端广播发送一个上线信息提示，
    //然后加入到“在线列表中”，但是自己本身也会收到，因为绑定和发送的IP以及端口都一样的
    if(this -> isStart){
        QString data = "#ack0#" + QString::number(this -> udpSocket -> socketDescriptor()) + "," + QTime::currentTime().toString();
        this -> onWriteDataGram(data);
        this -> isStart = false;
        ui -> plainTextEdit -> appendPlainText("[" + QString::number(this -> udpSocket -> socketDescriptor()) + "] 上线 " + QTime::currentTime().toString());
    }
}

UdpGroupChat::~UdpGroupChat()
{
    delete ui;
}

QString UdpGroupChat::getLocalIp(){
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

void UdpGroupChat::udpBind()
{

    QString localIP = this -> getLocalIp();
    qint64 port = 8080;

    QHostAddress addr(localIP);
    // this -> udpSocket -> bind(addr,port);
    /*共享相同的地址
        ReuseAddressHint： 向 QAbstractSocket 提供提示，即使地址和端口已经被另一个Socket绑定，它也应该尝试重新绑定服务。
        ShareAddress：     允许其他服务（其他进程的）绑定到相同的地址和端口。通过将此选项与
                           ReuseAddressHint 结合使用，将允许您的服务重新绑定现有的共享地址。
    */
    this -> udpSocket -> bind(port,QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);
}

void UdpGroupChat::onWriteDataGram(QString data)
{
    //也就是绑定不仅用的是本机地址和8080端口，同时发送消息使用的也就是本机地址和8080端口，
    //因为实现群聊的话，消息不仅要让其他好友的窗口看到，同时自己也要窗口显示
    QHostAddress addr(this -> getLocalIp());
    quint16 port = 8080;

    //如果选择广播方式的话，则给所有广播地址发送消息；否则只给指定的端口和指定的IP主机发送消息
    QHostAddress broadcastAddress = QHostAddress::Broadcast;
    // ui -> plainTextEdit -> appendPlainText("广播地址: " + broadcastAddress.toString());
    this -> udpSocket -> writeDatagram(data.toStdString().c_str(),data.size(),broadcastAddress,port);
}

void UdpGroupChat::onReadDataGram(){
    while(this -> udpSocket -> hasPendingDatagrams()){
        QByteArray datagram;
        datagram.resize(this -> udpSocket -> pendingDatagramSize());

        QHostAddress addr;
        quint16 port;

        this -> udpSocket -> readDatagram(datagram.data(),datagram.size(),&addr,&port);
        // ui -> plainTextEdit -> appendPlainText(QString("IP: %1  Port: %2").arg(addr.toString()).arg(port));
        //如果有新的用户上线的话，就读取上线用户发送的ACK确认信号并添加上线的用户；
        //但对于新上线的用户却没有得到当前用户的信息，因此，当前用户也需要发送一个ACK信号给其他在线用户
        if(QString(datagram).contains("#ack0#")){
            QString socketid_and_Time = QString(datagram).mid(6);//去掉前面字符串"#ack0#"，提取之后的字符串
            int socketID = socketid_and_Time.split(",")[0].toInt();
            QString time = socketid_and_Time.split(",")[1];
            this -> map[socketID] = time;
            //也需要其他新在线的用户发送当前在线用户的ID以及在线时间，以便于所有用户的“在线用户表”同时更新
            this -> onWriteDataGram("#ack1#" + QString::number(this -> udpSocket -> socketDescriptor()) + ","  + this -> map.value(this -> udpSocket -> socketDescriptor()));
        }else if(QString(datagram).contains("#ack1#")){
            QString socketid_and_Time = QString(datagram).mid(6);//去掉前面字符串"#ack1#"，提取之后的字符串
            int socketID = socketid_and_Time.split(",")[0].toInt();
            QString time = socketid_and_Time.split(",")[1];
            //这里需要判断一下，如果当前在线的用户已经存在于“在线用户表”中就不添加
            if(!this -> isExistMap(socketID)){
                this -> map[socketID] = time;
            }
        }else if(QString(datagram).contains("#exit#")){
            QString socketid_and_Time = QString(datagram).mid(6);//去掉前面字符串"#exit#"，提取之后的字符串
            int socketID = socketid_and_Time.split(",")[0].toInt();
            QString time = socketid_and_Time.split(",")[1];
            this -> map[socketID] = time;
        }else{
            int socketID = QString(datagram).split("#")[0].toInt();
            QString msg = QString(datagram).split("#")[1];
            ui -> plainTextEdit -> appendPlainText("[好友: " + QString::number(socketID) + "] " + msg);
        }

        this -> addTableWidgetItem();
    }
}

bool UdpGroupChat::isExistMap(int socketID){
    for(const auto & key : this -> map.keys()){
        if(key == socketID){
            return true;
        }
    }
    return false;
}

void UdpGroupChat::addTableWidgetItem(){
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

void UdpGroupChat::on_btn_send_clicked(){
    QString data = ui -> textEdit -> toPlainText();
    ui -> textEdit -> clear();
    //发送的消息里面包含了当前发送者的socketID以及消息内容本身
    int socketID = this -> udpSocket -> socketDescriptor();
    data = QString::number(socketID) +"#"+ data;
    this -> onWriteDataGram(data);
}

void UdpGroupChat::on_btn_close_clicked(){
    //如果当前用户退出的话，也需要给其他用户发送消息机制，更新“在线用户表”
    int socketID = this -> udpSocket -> socketDescriptor();
    QString time = "0";
    this -> onWriteDataGram("#exit#" + QString::number(socketID) + "," + time);

    this -> udpSocket -> abort();
    this -> udpSocket -> deleteLater();
    this -> close();
}

