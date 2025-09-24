#ifndef SERVER_H
#define SERVER_H

#include <QMainWindow>
#include <QAbstractSocket>
#include <QHostInfo>
#include <QMessageBox>
#include <QNetworkInformation>
#include <QNetworkInterface>
#include <QFile>
#include <QLayout>
#include <iostream>
#include <QTimer>

#include "ledindicator.h"
#include "networkprotocol.h"
#include "websocketserver.h"

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

    QString getSocketStateString(QAbstractSocket::SocketState state);

private slots:
    void on_listen_btn_clicked();

    void on_close_btn_clicked();

    void on_search_btn_clicked();

    void on_exit_btn_clicked();

    // WebSocketServer信号槽
    void onServerStarted();
    void onServerStopped();
    void onClientConnected(const QString &clientId);
    void onClientDisconnected(const QString &clientId);
    void onErrorOccurred(const QString &errorMessage);
    void onMessageReceived(const NetworkMessage &message);

    void diplayClientCounts();

private:
    Ui::Server *ui;

    LedIndicator *m_connectionStatus;

    WebSocketServer *m_webSocketServer;  // WebSocket服务器实例
    bool m_isServerRunning;              // 服务器运行状态

    void updateServerStatus(bool isRunning);
    void appendLog(const QString &message);
    void setupConnections();

    // 定时更新客户端连接数量
    QTimer * m_timer;
};
#endif // SERVER_H
