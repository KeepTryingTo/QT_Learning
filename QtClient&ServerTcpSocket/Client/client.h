#ifndef CLIENT_H
#define CLIENT_H

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

public slots:
    void readSocket();
    void discardSocket();
    void sendFile(QTcpSocket *socket, QString fileName);

    void onSocketStateChange(QAbstractSocket::SocketState socketState);


private slots:
    void on_btn_connect_clicked();

    void on_btn_disconnect_clicked();

    void on_btn_close_clicked();

    void on_btn_send_file_clicked();

    void on_pushButton_clicked();

private:
    Ui::Client *ui;

    QString fileDir;
    QTcpSocket * socket;
};
#endif // CLIENT_H
