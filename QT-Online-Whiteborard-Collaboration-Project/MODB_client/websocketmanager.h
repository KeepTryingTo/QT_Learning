#ifndef WEBSOCKETMANAGER_H
#define WEBSOCKETMANAGER_H

#include <QObject>
#include <QtWebSockets/QtWebSockets>
#include <QTimer>
#include <iostream>
#include "networkprotocol.h"

class WebSocketManager : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketManager(QObject *parent = nullptr);
    ~WebSocketManager();

    // 连接服务器，断开连接以及判断是否连接
    bool connectToServer(const QString &url);
    void disconnectFromServer();
    bool isConnected() const{return this ->m_isConnected;};

    // 加入一个房间，离开房间
    void joinRoom(const QString &roomId, const QString &userName);
    void leaveRoom();

    // 发送绘制图形操作
    void sendDrawingOperation(const DrawingOperation &operation);
    // 发送清除绘图场景
    void sendClearScene();
    // 发送撤销操作
    void sendUndoRequest();
    // 发送重做操作
    void sendRedoRequest();
    // 发送聊天信息
    void sendChatMessage(const QString &message);

    void sendNetworkMessage(const NetworkMessage &message);
    // 房间管理相关方法
    QString getCurrentRoomName() const;
    QList<QPair<QString, QString>> getAvailableRooms() const;
    // 当前房间号和当前用户id
    QString getCurrentRoomId();
    QString getUserId();
    UserRole getCurrentRole();

    // 添加房间创建方法
    QString createRoom(const QString &roomName);

    QWebSocket* getSocket(){
        return this -> m_webSocket;
    }

signals:
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    void userLeft(const QString &userId);
    void clientListReceived(const QList<QJsonObject> &clients);
    void userJoined(const QString &userId, const QString &userName, int role);

    void drawingOperationReceived(const DrawingOperation &operation);
    void clearSceneReceived();
    void undoRequestReceived();
    void redoRequestReceived();
    void chatMessageReceived(const QString &userId, const QString &message);
    void userRoleChanged(const QString &userId, UserRole newRole);

    void joinedRoom(const QString &roomId, const QString &userId);
    void roomJoined(const QString&roomId, const QString & roomName);
    void roomCreated(const QString &roomId, const QString & roomName);
    void roomLeft(const QString& roomId);
    void roomListReceived(const QList<QPair<QString, QString>> &rooms);
    void roomError(const QString&errorMessage);


private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void sendHeartbeat();

private:
    QWebSocket *m_webSocket;
    QTimer *m_heartbeatTimer;
    QString m_userId;
    QString m_userName;
    QString m_roomId;
    UserRole m_currentRole;
    bool m_isConnected;

    // 添加房间相关成员变量
    QString m_currentRoomId;
    QString m_currentRoomName;
    QMap<QString, QString> m_availableRooms; // roomId -> roomName

    void processMessage(const NetworkMessage &message);
    void processClientList(const QJsonObject &data);

    // 存储房间内的用户信息
    QMap<QString, QJsonObject> m_roomClients;
};

#endif // WEBSOCKETMANAGER_H
