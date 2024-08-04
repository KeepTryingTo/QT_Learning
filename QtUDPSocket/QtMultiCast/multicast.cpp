#include "multicast.h"
#include "ui_multicast.h"

MultiCast::MultiCast(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MultiCast)
{
    ui->setupUi(this);

    this -> udpSocket = new QUdpSocket(this);
    QString localIP = this -> getLocalIp();
    ui -> lineEdit_IP -> setText(localIP);

    ui -> lineEdit_multi_ip -> setText("239.255.0.2");

    connect(this -> udpSocket,&QUdpSocket::readyRead,this,&MultiCast::onReadDataGram);
}

MultiCast::~MultiCast()
{
    delete ui;
}

QString MultiCast::getLocalIp(){
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

void MultiCast::udpBind(){
    qint64 port = ui -> spinBox_senderPort -> value();
    QHostAddress multiCastAddress(ui -> lineEdit_multi_ip -> text().trimmed());

    if(ui -> checkBox -> isChecked()){
        this -> udpSocket -> bind(QHostAddress::AnyIPv4,port); //ReuseAddressHint 结合使用，将允许您的服务重新绑定现有的共享地址
        //将目标地址加入多播组中
        this -> udpSocket -> joinMulticastGroup(multiCastAddress);
    }else{
        this -> udpSocket -> leaveMulticastGroup(multiCastAddress);
        QString localIP = ui -> lineEdit_IP -> text().trimmed();
        QHostAddress addr(localIP);
        this -> udpSocket -> bind(addr,port);
    }
}

void MultiCast::onWriteDataGram(QString data)
{
    QHostAddress addr(ui -> lineEdit_IP -> text().trimmed());
    quint16 port = ui -> spinBox_receiverPort -> value();

    //如果选择广播方式的话，则给所有广播地址发送消息；否则只给指定的端口和指定的IP主机发送消息
    if(ui -> checkBox -> isChecked()){
        qDebug()<<ui -> checkBox -> isChecked();
        QHostAddress multiCastAddress(ui -> lineEdit_multi_ip -> text().trimmed());
        ui -> plainTextEdit -> appendPlainText("多播地址: " + multiCastAddress.toString());
        this -> udpSocket -> writeDatagram(data.toStdString().c_str(),data.size(),multiCastAddress,port);
    }else{
        qDebug()<<ui -> checkBox -> isChecked();
        this -> udpSocket -> writeDatagram(data.toStdString().c_str(),data.size(),addr,port);
    }
}

void MultiCast::onReadDataGram()
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

void MultiCast::on_btn_bind_clicked()
{
    this -> udpBind();
    ui -> btn_bind -> setEnabled(false);
}


void MultiCast::on_btn_send_clicked()
{
    QString data = ui -> lineEdit -> text();
    ui -> lineEdit -> clear();
    this -> onWriteDataGram(data);
}


void MultiCast::on_btn_close_clicked()
{
    this -> udpSocket -> abort();
    this -> udpSocket -> deleteLater();
    this -> close();
}

