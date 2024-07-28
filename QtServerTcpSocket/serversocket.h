#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include <QMainWindow>
#include <QString>
#include <QMessageBox>
#include <QList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QHostInfo>

QT_BEGIN_NAMESPACE
namespace Ui {
class ServerSocket;
}
QT_END_NAMESPACE

class ServerSocket : public QMainWindow
{
    Q_OBJECT

public:
    ServerSocket(QWidget *parent = nullptr);
    ~ServerSocket();

    QString getLocalIp();
    void initState();
    void AddNewClientConnection(QTcpSocket * socket);

public slots:
    void onDisconnected();
    void onreadyReady();
    void onError(QAbstractSocket::SocketError socketError);
    void onSocketStateChange(QAbstractSocket::SocketState socketState);

private slots:
    void on_btn_close_clicked();

    void on_btn_clear_clicked();

    void on_btn_disconnect_clicked();

    void on_btn_listen_clicked();

    void on_btn_send_clicked();

    void onNewConnection();

private:
    Ui::ServerSocket *ui;

    QTcpServer * server;
    QList<QTcpSocket*> sockets;

};
#endif // SERVERSOCKET_H
