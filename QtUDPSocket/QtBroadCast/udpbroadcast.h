#ifndef UDPBROADCAST_H
#define UDPBROADCAST_H

#include <QMainWindow>

#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QHostInfo>

#include <QFile>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class UdpBroadCast;
}
QT_END_NAMESPACE

class UdpBroadCast : public QMainWindow
{
    Q_OBJECT

public:
    UdpBroadCast(QWidget *parent = nullptr);
    ~UdpBroadCast();

    QString getLocalIp();
    void udpBind();
    void onWriteDataGram(QString data);

private slots:
    void on_btn_close_clicked();

    void on_btn_bind_clicked();

    void on_btn_send_clicked();

    void onReadDataGram();

private:
    Ui::UdpBroadCast *ui;

    QUdpSocket * udpSocket;
};
#endif // UDPBROADCAST_H
