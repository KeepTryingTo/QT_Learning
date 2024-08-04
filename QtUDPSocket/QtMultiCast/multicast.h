#ifndef MULTICAST_H
#define MULTICAST_H

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
class MultiCast;
}
QT_END_NAMESPACE

class MultiCast : public QMainWindow
{
    Q_OBJECT

public:
    MultiCast(QWidget *parent = nullptr);
    ~MultiCast();

    QString getLocalIp();
    void udpBind();
    void onWriteDataGram(QString data);


private slots:
    void on_btn_bind_clicked();

    void on_btn_send_clicked();

    void on_btn_close_clicked();

    void onReadDataGram();

private:
    Ui::MultiCast *ui;

    QUdpSocket * udpSocket;
};
#endif // MULTICAST_H
