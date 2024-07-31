#ifndef NETWORKWIFIMANAGER_H
#define NETWORKWIFIMANAGER_H

#include <QMainWindow>

#include "wifi_manager.h"

#include <QTimer>
#include <QTime>
#include <QFile>

QT_BEGIN_NAMESPACE
namespace Ui {
class NetworkWifiManager;
}
QT_END_NAMESPACE

class NetworkWifiManager : public QMainWindow
{
    Q_OBJECT

public:
    NetworkWifiManager(QWidget *parent = nullptr);
    ~NetworkWifiManager();

public slots:
    void TimerWifiScanFunction();

private:
    Ui::NetworkWifiManager *ui;

    Wifi_Manager * wifi_manager;
    QTimer * TimerWifiScan;
};
#endif // NETWORKWIFIMANAGER_H
