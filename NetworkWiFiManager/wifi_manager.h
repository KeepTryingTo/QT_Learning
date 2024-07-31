#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <QNetworkAccessManager>
#include <QtGui>
#include <QtCore>
#include <QtWidgets>

class Wifi_Manager
{
public:
    Wifi_Manager();

    void ScanWifiNetwork();
    QStringList GetAvailableWifiNetwork();

private:
    QStringList AvailableWifiNetwork;
};

#endif // WIFI_MANAGER_H
