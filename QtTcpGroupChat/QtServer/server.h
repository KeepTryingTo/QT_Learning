#ifndef SERVER_H
#define SERVER_H

#include <QMainWindow>

#include <QString>
#include <QMessageBox>
#include <QList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QHostInfo>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui {
class Server;
}
QT_END_NAMESPACE

class Server : public QMainWindow
{
    Q_OBJECT

public:
    Server(QWidget *parent = nullptr);
    ~Server();

    QString getLocalIp();
    void initState();
    void AddNewClientConnection(QTcpSocket * socket);
    void send_to_socket();
    void server_listen();
    bool isContains(int id);//用于判断当前的客户端ID是否存和服务端连接;如果是连接的，则返回true，否则返回false
    void sendMapData();//用于发送map保存的数据
    void onSleep(unsigned int milliseconds);

public slots:
    void onDisconnected();
    void onreadyReady();
    void onNewConnection();
    void onError(QAbstractSocket::SocketError socketError);
    void onSocketStateChange(QAbstractSocket::SocketState socketState);

private slots:
    void on_pushButton_clicked();

private:
    Ui::Server *ui;

    QString info;

    QTcpServer * server;
    QList<QTcpSocket*> sockets;
    QMap<int,QString>map; //用于保存当前哪些客户端和服务端是建立连接
};
#endif // SERVER_H
