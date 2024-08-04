#include "udpclient.h"
#include "ui_udpclient.h"

UdpClient::UdpClient(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::UdpClient)
{
    ui->setupUi(this);

    this -> udpSocket = new QUdpSocket(this);

    QString localIP = this -> getLocalIp();
    ui -> lineEdit -> setText(localIP);

    connect(this -> udpSocket,&QUdpSocket::readyRead,this,&UdpClient::onReadDataGram);
}

UdpClient::~UdpClient()
{
    delete ui;
}

QString UdpClient::getLocalIp(){
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

void UdpClient::udpBind()
{
    QString localIP = ui -> lineEdit -> text();
    qint64 port = ui -> spinBox_senderPort -> value();

    QHostAddress addr(localIP);
    this -> udpSocket -> bind(addr,port);
}

void UdpClient::onWriteDataGram(QString data)
{
    QHostAddress addr(ui -> lineEdit -> text());
    quint16 port = ui -> spinBox_receiverPort -> value();

    this -> udpSocket -> writeDatagram(data.toStdString().c_str(),data.size(),addr,port);
}

void UdpClient::onReadDataGram()
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

void UdpClient::on_btn_bind_clicked()
{
    this -> udpBind();
    ui -> btn_bind -> setEnabled(false);
}


void UdpClient::on_btn_send_clicked()
{
    QString data = ui -> textEdit -> toPlainText();
    this -> onWriteDataGram(data);
    ui -> textEdit -> clear();
}


void UdpClient::on_btn_close_clicked()
{
    this -> udpSocket -> abort();//断开连接，中断套接字
    this -> close();
}


