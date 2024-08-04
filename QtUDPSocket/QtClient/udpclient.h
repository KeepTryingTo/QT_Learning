#ifndef UDPCLIENT_H
#define UDPCLIENT_H

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
class UdpClient;
}
QT_END_NAMESPACE

class UdpClient : public QMainWindow
{
    Q_OBJECT

public:
    UdpClient(QWidget *parent = nullptr);
    ~UdpClient();

    QString getLocalIp();
    void udpBind();
    void onWriteDataGram(QString data);

private slots:
    void on_btn_bind_clicked();

    void on_btn_send_clicked();

    void on_btn_close_clicked();

    void onReadDataGram();

private:
    Ui::UdpClient *ui;

    QUdpSocket * udpSocket;
};
#endif // UDPCLIENT_H
