#ifndef SERVER_H
#define SERVER_H

#include <QMainWindow>

#include <QString>
#include <QFile>
#include <QTcpSocket>
#include <QTcpServer>
#include <QFileInfo>
#include <QFileDialog>
#include <QHostInfo>
#include <QHostAddress>


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

public slots:
    void readSocket();
    void discardSocket();

    void newConnection();
    void AddToSocketList(QTcpSocket * socket);

    void sendFile(QTcpSocket * socket,QString fileName);
    void onreadyReady();

    void onSocketStateChange(QAbstractSocket::SocketState socketState);

private slots:
    void on_btn_listen_clicked();

    void on_btn_disconnect_clicked();

    void on_btn_send_file_clicked();

    void on_btn_close_clicked();

    void on_btn_save_clicked();

private:
    Ui::Server *ui;
    QString fileDir;//针对来自客户端发送的文件信息，服务端选择保存的路径

    QTcpServer * server;
    QList<QTcpSocket*>client_list;
};
#endif // SERVER_H
