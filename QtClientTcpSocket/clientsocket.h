#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QHostAddress>
#include <QNetworkAccessManager>

QT_BEGIN_NAMESPACE
namespace Ui {
class ClientSocket;
}
QT_END_NAMESPACE

class ClientSocket : public QMainWindow
{
    Q_OBJECT

public:
    ClientSocket(QWidget *parent = nullptr);
    ~ClientSocket();

    void initSocket();
    QString getLocalIp();

private slots:
    void on_btn_close_clicked();

    void on_btn_send_clicked();

    void on_btn_disconnect_clicked();

    void on_btn_connect_clicked();

    void on_btn_clear_clicked();

    void onSocketStateChange(QAbstractSocket::SocketState socketState);
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onreadyReady();

private:
    Ui::ClientSocket *ui;

    QTcpSocket * socket;
};
#endif // CLIENTSOCKET_H
