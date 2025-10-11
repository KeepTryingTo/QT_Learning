#pragma once
#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QtWebSockets/QtWebSockets>
#include <QUrl>
#include <iostream>
#include "networkprotocol.h"
#include "piecetype.h"

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
    void sendGameStart(const QString &opponent, PieceType myPieceType);
    void sendGameEnd(const QString &winner);
    void sendChatMessage(const QString& sender, const QString &message);

    bool isConnected() const;
    QString getUsername() const;

    // 创建新的房间，加入创建的房间，离开房间以及请求当前房间列表
    void createRoom(const QString &roomName, const QString &roomId);
    void joinRoom(const QString &roomId);
    void leaveRoom(const QString &roomId);
    void requestRoomList();

    void handleUserListMessage(const QJsonArray &users);
    void requestUserList();

    void sendStatsUpdate(const QString &username, int score,
                         int wins, int losses, int draws, int streak);

    void sendResumeMessage(const QString& roomId);
    void sendGiveUpMessage(const QString& username, const QString& roomId);
    void sendPeaceMessage();

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);

    void loginSuccess(const QString &username);
    void loginFailed(const QString &error);
    void logoutReceived(const QString &username);
    void userStatusChanged(const QString &username, const QString &action);

    // 接收到的消息信号
    void messageReceived(const NetworkMessage &message);
    void moveReceived(int x, int y, PieceType piece);
    void gameStartReceived(const QString &opponent, PieceType myPieceType, PieceType opponentPieceType);
    // void gameStartReceived(const QString &opponent, PieceType myPieceType);  // 修改为带 PieceType
    void gameEndReceived(const QString &winner);
    void chatMessageReceived(const QString& sender, const QString &message);

    // 加入房间，创建房间
    void roomCreated(const QString &roomId, const QString &roomName);
    void roomJoined(const QString &roomId, const QString &roomName, const QStringList &members);
    void roomLeft(const QString &roomId);
    void roomListReceived(const QJsonArray &rooms);
    void roomError(const QString &errorMessage);

    void userJoined(const QString &username);
    void userLeft(const QString &username);
    void userListReceived(const QStringList &users);

    void statsUpdated(QString&username, int score,int  wins, int losses,int  draws, int streak);

    void sendResumeStart();
    void sendGiveUpsignal(const QString& username);
    void sendPeaceSignal();

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
