#include "networkwifimanager.h"
#include "ui_networkwifimanager.h"

NetworkWifiManager::NetworkWifiManager(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NetworkWifiManager)
{
    ui->setupUi(this);

    this -> setFixedSize(QSize(590,230));
    this -> setWindowTitle("WiFi Manager");

    this -> wifi_manager = new Wifi_Manager;
    this -> TimerWifiScan = new QTimer();

    connect(this -> TimerWifiScan,&QTimer::timeout,this,&NetworkWifiManager::TimerWifiScanFunction);
    this -> TimerWifiScan -> setInterval(1000);//间隔1秒之后再执行
    this -> TimerWifiScan -> start();


}

NetworkWifiManager::~NetworkWifiManager()
{
    delete ui;
}

void NetworkWifiManager::TimerWifiScanFunction(){
    this -> wifi_manager -> ScanWifiNetwork();
    QStringList wifiList = this -> wifi_manager -> GetAvailableWifiNetwork();
    ui -> WifiListWidget -> clear();
    ui -> WifiListWidget -> addItems(wifiList);
}
