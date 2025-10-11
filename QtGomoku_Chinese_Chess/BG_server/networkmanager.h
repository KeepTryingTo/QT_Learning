#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QtWebSockets/QtWebSockets>
#include <QUrl>
#include "networkprotocol.h"

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    // 连接服务器
    bool connectToServer(const QUrl &url);
    void disconnectFromServer();

    // 发送消息
    void sendMessage(const NetworkMessage &message);

    // 登录/登出
    void login(const QString &username);
    void logout();

    // 游戏相关
    void sendMove(int x, int y, PieceType piece);
    void sendGameStart(const QString &opponent);
    void sendGameEnd(const QString &winner);
    void sendChatMessage(const QString &message);

    bool isConnected() const;
    QString getUsername() const;
    void sendStatsUpdate(const QString &username, int score,
                         int wins, int losses, int draws, int streak);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);

    // 接收到的消息信号
    void messageReceived(const NetworkMessage &message);
    void moveReceived(int x, int y, PieceType piece);
    void gameStartReceived(const QString &opponent);
    void gameEndReceived(const QString &winner);
    void chatMessageReceived(const QString &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);

private:
    QWebSocket *m_webSocket;
    QUrl m_serverUrl;
    QString m_username;
    bool m_isConnected;
};

#endif // NETWORKMANAGER_H
