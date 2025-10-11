#ifndef SERVER_H
#define SERVER_H

#include <QMainWindow>

#include <QObject>
#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QtWebSockets>
#include <QMap>
#include <QLayout>
#include <QMessageBox>
#include <iostream>

#include "networkprotocol.h"
#include "ledindicator.h"

struct RoomInfo {
    QString roomId;
    QString roomName;
    QString creator;
    QSet<QString> members; // 房间成员用户名
    QDateTime createTime;
};

QT_BEGIN_NAMESPACE
namespace Ui {
class Server;
}
QT_END_NAMESPACE

class Server : public QMainWindow
{
    Q_OBJECT


private slots:
    void onNewConnection();
    void onMessageReceived(const QString &message);
    void onSocketDisconnected();

public:
    Server(QWidget *parent = nullptr);
    ~Server();

    // 获取在线用户列表
    QStringList getOnlineUsers() const;

    // 向特定用户发送消息
    bool sendToUser(const QString &username, const QJsonObject &message);

    QString getSocketStateString(QAbstractSocket::SocketState state);
    QString getLocalIp();

signals:
    void clientConnected(const QString &username);
    void clientDisconnected(const QString &username);
    void messageReceived(const QString &from, const QString &message);

private slots:
    void on_listen_btn_clicked();

    void on_close_btn_clicked();

    void on_search_btn_clicked();

    void on_exit_btn_clicked();

private:
    Ui::Server *ui;

    QWebSocketServer *m_server;
    QMap<QWebSocket*, QString> m_clients;          // socket -> username
    QMap<QString, QWebSocket*> m_usernameToSocket; // username -> socket

    void processMessage(QWebSocket *client, const QJsonObject &message);
    void broadcastMessage(const QJsonObject &message, QWebSocket *exclude = nullptr);
    void sendError(QWebSocket *client, const QString &errorMessage);
    void broadcastMessageToRoom(const QString &roomId, const QJsonObject &message, QWebSocket *exclude);
    void sendGameStartToUser(const QString &username, const QString &opponent, PieceType pieceType);

    // 更新UI显示
    void updateStatusDisplay();
    void appendToChatFrame(const QString &message);
    void updateOnlineUsersList();

    // 服务器状态
    bool m_isListening;

    LedIndicator* m_led;

    // 房间ID和房间信息以及用户信息和房间ID之间关系
    QMap<QString, RoomInfo> m_rooms; // roomId -> RoomInfo
    QMap<QString, QString> m_userToRoom; // username -> roomId
};
#endif // SERVER_H
