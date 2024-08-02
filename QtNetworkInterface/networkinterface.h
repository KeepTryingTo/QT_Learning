#ifndef NETWORKINTERFACE_H
#define NETWORKINTERFACE_H

#include <QMainWindow>

#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QNetworkAddressEntry>

#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class NetworkInterface;
}
QT_END_NAMESPACE

class NetworkInterface : public QMainWindow
{
    Q_OBJECT

public:
    NetworkInterface(QWidget *parent = nullptr);
    ~NetworkInterface();

    QString getLocalIp();
    QString protocolName(QAbstractSocket::NetworkLayerProtocol protocol);

private slots:
    void on_btn_all_ip_clicked();

    void on_btn_ipv4_clicked();

    void on_btn_all_interface_clicked();

    void on_btn_close_clicked();

    void on_btn_clear_clicked();

    void on_btn_analyze_clicked();

    void onLookedUpHostInfo(const QHostInfo &host);

private:
    Ui::NetworkInterface *ui;
};
#endif // NETWORKINTERFACE_H
