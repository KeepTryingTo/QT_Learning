#include "networkinterface.h"
#include "ui_networkinterface.h"

NetworkInterface::NetworkInterface(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NetworkInterface)
{
    ui->setupUi(this);
    ui -> lineEdit -> setText("paperwithcode.com");
}

NetworkInterface::~NetworkInterface()
{
    delete ui;
}

QString NetworkInterface::getLocalIp(){
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

void NetworkInterface::on_btn_all_ip_clicked(){
    for(const QHostAddress & address : QNetworkInterface::allAddresses()){
        ui -> plainTextEdit -> appendPlainText(address.toString());
    }
    ui -> plainTextEdit -> appendPlainText("---------------------------------------");
}


void NetworkInterface::on_btn_ipv4_clicked(){
    for(const QHostAddress & address : QNetworkInterface::allAddresses()){
        if(address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)){
            ui -> plainTextEdit -> appendPlainText(address.toString());
        }
    }
    ui -> plainTextEdit -> appendPlainText("---------------------------------------");
}


void NetworkInterface::on_btn_all_interface_clicked(){
    for(const QNetworkInterface & inter : QNetworkInterface::allInterfaces()){
        ui -> plainTextEdit -> appendPlainText("interface name: " + inter.name());
        ui -> plainTextEdit -> appendPlainText("interface humanReadableName: " + inter.humanReadableName());
        ui -> plainTextEdit -> appendPlainText("interface hardwareAddress: " + inter.hardwareAddress());
        ui -> plainTextEdit -> appendPlainText("interface type: " + QString::number(inter.type()));
        ui -> plainTextEdit -> appendPlainText("interface isValid: " + QString::number(inter.isValid()));
        ui -> plainTextEdit -> appendPlainText("###对应的IP地址，网络掩码以及广播地址如下：");
        //QNetworkAddressEntry是Qt网络模块中的一个类，它主要用于存储和管理网络接口所支持的IP地址，以及这些地址相关的网络掩码和广播地址信息
        QList<QNetworkAddressEntry> entry = inter.addressEntries();
        foreach (QNetworkAddressEntry ent, entry) {
            ui->plainTextEdit->appendPlainText("  IP 地址：" + ent.ip().toString());
            ui->plainTextEdit->appendPlainText("  子网掩码：" + ent.netmask().toString());
            ui->plainTextEdit->appendPlainText("  子网广播：" + ent.broadcast().toString() + "\n");
        }
    }
    ui -> plainTextEdit -> appendPlainText("---------------------------------------");
}


void NetworkInterface::on_btn_close_clicked(){
    this -> close();
}


void NetworkInterface::on_btn_clear_clicked(){
    ui -> plainTextEdit -> clear();
}

void NetworkInterface::on_btn_analyze_clicked(){
    QString hostName = ui->lineEdit->text();  //域名
    ui -> lineEdit -> clear();
    ui->plainTextEdit->appendPlainText("正在查找域名的服务器的主机信息：" + hostName);
    //域名的名称，接收者以及成员
    QHostInfo::lookupHost(hostName, this, SLOT(onLookedUpHostInfo(QHostInfo)));
}

QString NetworkInterface::protocolName(QAbstractSocket::NetworkLayerProtocol protocol){
    switch ((protocol)) {
    case QAbstractSocket::IPv4Protocol:
        return "IPv4 Protocol";
    case QAbstractSocket::IPv6Protocol:
        return "IPv6 Protocol";
    case QAbstractSocket::AnyIPProtocol:
        return "Any IP Protocol";
    default:
        return "Unknow Network Layer Protocol";
    }
}

void NetworkInterface::onLookedUpHostInfo(const QHostInfo &host)
{
    QList<QHostAddress> list = host.addresses();
    if (list.isEmpty()){
        ui -> plainTextEdit -> appendPlainText("为查询到相关信息");
        return;
    }
    for (int i = 0; i < list.count(); i++) {
        QHostAddress host = list.at(i);
        ui->plainTextEdit->appendPlainText("协议：" + protocolName(host.protocol()));
        ui->plainTextEdit->appendPlainText("IP：" + host.toString());
    }
}

