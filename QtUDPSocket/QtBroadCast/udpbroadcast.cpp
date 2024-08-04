#include "udpbroadcast.h"
#include "ui_udpbroadcast.h"

UdpBroadCast::UdpBroadCast(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::UdpBroadCast)
{
    ui->setupUi(this);

    this -> udpSocket = new QUdpSocket(this);
    QString localIP = this -> getLocalIp();
    ui -> lineEdit_IP -> setText(localIP);

    connect(this -> udpSocket,&QUdpSocket::readyRead,this,&UdpBroadCast::onReadDataGram);
}

UdpBroadCast::~UdpBroadCast()
{
    delete ui;
}

QString UdpBroadCast::getLocalIp(){
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

void UdpBroadCast::udpBind()
{
    QString localIP = ui -> lineEdit_IP -> text().trimmed();
    qint64 port = ui -> spinBox_senderPort -> value();

    QHostAddress addr(localIP);
    // this -> udpSocket -> bind(addr,port);
    /*共享相同的地址
        ReuseAddressHint： 向 QAbstractSocket 提供提示，即使地址和端口已经被另一个Socket绑定，它也应该尝试重新绑定服务。
        ShareAddress：     允许其他服务（其他进程的）绑定到相同的地址和端口。通过将此选项与
                           ReuseAddressHint 结合使用，将允许您的服务重新绑定现有的共享地址。
    */
    this -> udpSocket -> bind(port,QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);
}

void UdpBroadCast::onWriteDataGram(QString data)
{
    QHostAddress addr(ui -> lineEdit_IP -> text().trimmed());
    quint16 port = ui -> spinBox_receiverPort -> value();

    //如果选择广播方式的话，则给所有广播地址发送消息；否则只给指定的端口和指定的IP主机发送消息
    if(ui -> checkBox -> isChecked()){
        qDebug()<<ui -> checkBox -> isChecked();
        QHostAddress broadcastAddress = QHostAddress::Broadcast;
        ui -> plainTextEdit -> appendPlainText("广播地址: " + broadcastAddress.toString());
        this -> udpSocket -> writeDatagram(data.toStdString().c_str(),data.size(),broadcastAddress,port);
    }else{
        qDebug()<<ui -> checkBox -> isChecked();
        this -> udpSocket -> writeDatagram(data.toStdString().c_str(),data.size(),addr,port);
    }
}

void UdpBroadCast::onReadDataGram()
{
    while(this -> udpSocket -> hasPendingDatagrams()){
        QByteArray datagram;
        datagram.resize(this -> udpSocket -> pendingDatagramSize());

        QHostAddress addr;
        quint16 port;

        this -> udpSocket -> readDatagram(datagram.data(),datagram.size(),&addr,&port);
        ui -> plainTextEdit -> appendPlainText(QString("IP: %1  Port: %2").arg(addr.toString()).arg(port));
        ui -> plainTextEdit -> appendPlainText(QString(datagram));
    }
}

void UdpBroadCast::on_btn_close_clicked()
{
    this -> udpSocket -> abort();
    this -> udpSocket -> deleteLater();
    this -> close();
}


void UdpBroadCast::on_btn_bind_clicked()
{
    this -> udpBind();
    ui -> btn_bind -> setEnabled(false);
}


void UdpBroadCast::on_btn_send_clicked()
{
    QString data = ui -> lineEdit -> text();
    ui -> lineEdit -> clear();
    this -> onWriteDataGram(data);
}



