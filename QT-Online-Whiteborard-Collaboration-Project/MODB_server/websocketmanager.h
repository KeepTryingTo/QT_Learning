#ifndef WEBSOCKETMANAGER_H
#define WEBSOCKETMANAGER_H

#include <QObject>
#include <QtWebSockets/QtWebSockets>
#include <QTimer>
#include "networkprotocol.h"

class WebSocketManager : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketManager(QObject *parent = nullptr);
    ~WebSocketManager();

    bool connectToServer(const QString &url);
    void disconnectFromServer();
    bool isConnected() const;

    void joinRoom(const QString &roomId, const QString &userName);
    void leaveRoom();
    void sendDrawingOperation(const DrawingOperation &operation);
    void sendClearScene();
    void sendUndoRequest();
    void sendRedoRequest();
    void sendChatMessage(const QString &message);

    QString getCurrentRoomId() const;
    QString getUserId() const;
    UserRole getCurrentRole() const;

signals:
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    void joinedRoom(const QString &roomId, const QString &userId);
    void userJoined(const QString &userId, const QString &userName, UserRole role);
    void userLeft(const QString &userId);
    void drawingOperationReceived(const DrawingOperation &operation);
    void clearSceneReceived();
    void undoRequestReceived();
    void redoRequestReceived();
    void chatMessageReceived(const QString &userId, const QString &message);
    void userRoleChanged(const QString &userId, UserRole newRole);

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

    void processMessage(const NetworkMessage &message);
    void sendNetworkMessage(const NetworkMessage &message);
};

#endif // WEBSOCKETMANAGER_H
