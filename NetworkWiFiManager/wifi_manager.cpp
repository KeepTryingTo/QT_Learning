#include "wifi_manager.h"

Wifi_Manager::Wifi_Manager() {}

void Wifi_Manager::ScanWifiNetwork()
{
    QProcess process;
    //返回所以可利用WiFi设备
    process.start("netsh",QStringList()<<"wlan"<<"show"<<"network");
    process.waitForFinished();

    QByteArray RawData = process.readAllStandardOutput();
    QString Data(RawData);

    this -> AvailableWifiNetwork.clear();
    QStringList DataLines = Data.split('\n');
    for(const QString&line : DataLines){
        if(line.trimmed().startsWith("SSID")){
            QString SSID = line.section(':',1).trimmed();
            if(SSID.isEmpty() == false){
                this -> AvailableWifiNetwork.append(SSID);
            }
        }
    }
}

QStringList Wifi_Manager::GetAvailableWifiNetwork()
{
    return this -> AvailableWifiNetwork;
}
