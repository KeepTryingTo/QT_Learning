#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>

#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QList>
#include <QFileInfoList>
#include <QFileInfo>
#include <QFile>
#include <QSize>
#include <QFont>
#include <QMessageBox>
#include <QDebug>
#include <QHostInfo>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui {
class Client;
}
QT_END_NAMESPACE

class Client : public QMainWindow
{
    Q_OBJECT

public:
    Client(QWidget *parent = nullptr);
    ~Client();

    QString getLocalIp();
    void addTableWidgetItem();

private slots:
    void on_btn_send_clicked();

    void on_btn_close_clicked();

    void onSocketStateChange(QAbstractSocket::SocketState socketState);
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

private:
    Ui::Client *ui;

    QTcpSocket * socket;
    QMap<int,QString>map;
};
#endif // CLIENT_H
