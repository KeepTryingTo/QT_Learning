#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QtWebSockets>
#include <QMap>
#include <map>
#include <QSet>
#include <QString>
#include <QMessageBox>
#include <iostream>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUuid>
#include "networkprotocol.h"

class WebSocketServer : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketServer(QObject *parent = nullptr);
    ~WebSocketServer();

    bool startServer(QString ip, quint16 port);
    void stopServer();
    bool isRunning() const;
    int getClientCount() const{return m_clients.size();};

    // 房间管理
    QString createRoom(const QString &roomName);
    bool removeRoom(const QString &roomId);
    QList<QString> getRoomList() const;

    void sendError(QWebSocket *socket, const QString &errorMessage);
    void processRoomListRequest(QWebSocket *socket);
    void processLeaveRequest(QWebSocket *socket, const QJsonObject &data);
    void processHeartbeat(QWebSocket *socket, const QJsonObject &data);
    void processUserRoleChange(QWebSocket *socket, const QJsonObject &data);
    void processChatMessage(QWebSocket *socket, const QJsonObject &data);
    void processUndoRequest(QWebSocket *socket, const QJsonObject &data);
    void processClearScene(QWebSocket *socket, const QJsonObject &data);
    void processCreateRoomRequest(QWebSocket *socket, const QJsonObject &data);
    void processRedoRequest(QWebSocket *socket, const QJsonObject &data);

signals:
    void serverStarted();
    void serverStopped();
    void clientConnected(const QString &clientId);
    void clientDisconnected(const QString &clientId);
    void messageReceived(const NetworkMessage &message);
    void errorOccurred(const QString &errorMessage);

public slots:
    void broadcastMessage(const NetworkMessage &message, const QString &excludeClientId = "");

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onTextMessageReceived(const QString &message);

private:
    // 每一个客户端对应的信息，一个客户端可以加入多个房间
    struct ClientInfo {
        QWebSocket *socket;
        QString userId;
        QString userName;
        UserRole role;
        QString roomId;
        QDateTime lastActive; // 最后活动时间
    };
    // 每一个房间对应的信息，包括有哪些客户端，一个房间可以有多个客户端
    struct RoomInfo {
        QString roomId;
        QString roomName;
        QSet<QString> clientIds;
        QJsonArray drawingHistory; // 绘图历史记录
        QList<QJsonObject> undoStack;       // 撤销栈
        QList<QJsonObject> redoStack;       // 重做栈
    };

    QWebSocketServer *m_webSocketServer;
    QMap<QWebSocket*, ClientInfo> m_clients;
    QMap<QString, RoomInfo> m_rooms;
    QMap<QString, QWebSocket*> m_clientSockets;

    void handleClientMessage(QWebSocket *socket, const NetworkMessage &message);
    void processJoinRequest(QWebSocket *socket, const QJsonObject &data);
    void processDrawingOperation(QWebSocket *socket, const QJsonObject &data);
    void removeOperationFromHistory(const QString &roomId, const QString &operationId);
    // 接收到来自客户端的数据之后，需要将数据同步到其他的客户端
    void sendToClient(QWebSocket *socket, const NetworkMessage &message);
    // 指定的客户端ID
    QString generateClientId() const;
    QString generateRoomId() const;

    // 添加清理不活跃客户端的方法
    void cleanupInactiveClients();

    bool is_join_room;
};

#endif // WEBSOCKETSERVER_H
