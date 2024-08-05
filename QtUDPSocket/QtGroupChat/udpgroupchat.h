#ifndef UDPGROUPCHAT_H
#define UDPGROUPCHAT_H

#include <QMainWindow>

#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QHostInfo>

#include <QFile>
#include <QMessageBox>
#include <QMap>
#include <QTime>

QT_BEGIN_NAMESPACE
namespace Ui {
class UdpGroupChat;
}
QT_END_NAMESPACE

class UdpGroupChat : public QMainWindow
{
    Q_OBJECT

public:
    UdpGroupChat(QWidget *parent = nullptr);
    ~UdpGroupChat();

    QString getLocalIp();
    void udpBind();
    void onWriteDataGram(QString data);
    void addTableWidgetItem();
    bool isExistMap(int socketID);

private slots:
    void on_btn_send_clicked();

    void on_btn_close_clicked();

    void onReadDataGram();

private:
    Ui::UdpGroupChat *ui;

    bool isStart = true;
    QMap<int,QString>map;
    QUdpSocket * udpSocket;
};
#endif // UDPGROUPCHAT_H
