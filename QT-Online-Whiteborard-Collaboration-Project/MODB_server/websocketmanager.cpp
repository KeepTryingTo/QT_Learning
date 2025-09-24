#include "websocketmanager.h"
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>

WebSocketManager::WebSocketManager(QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket())
    , m_heartbeatTimer(new QTimer(this))
    , m_userId("")
    , m_userName("")
    , m_roomId("")
    , m_currentRole(UR_Editor)
    , m_isConnected(false)
{
    connect(m_webSocket, &QWebSocket::connected, this, &WebSocketManager::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &WebSocketManager::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketManager::onTextMessageReceived);
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, &WebSocketManager::onErrorOccurred);

    m_heartbeatTimer->setInterval(30000); // 30秒心跳
    connect(m_heartbeatTimer, &QTimer::timeout, this, &WebSocketManager::sendHeartbeat);
}

WebSocketManager::~WebSocketManager()
{
    disconnectFromServer();
}

bool WebSocketManager::connectToServer(const QString &url)
{
    if (m_isConnected) {
        return true;
    }

    QUrl serverUrl(url);
    if (!serverUrl.isValid()) {
        emit connectionError("Invalid server URL");
        return false;
    }

    m_webSocket->open(serverUrl);
    return true;
}

void WebSocketManager::disconnectFromServer()
{
    if (m_isConnected) {
        // leaveRoom();
        m_webSocket->close();
        m_heartbeatTimer->stop();
    }
}

void WebSocketManager::onConnected()
{
    m_isConnected = true;
    m_heartbeatTimer->start();
    emit connected();
}

void WebSocketManager::onDisconnected()
{
    m_isConnected = false;
    m_heartbeatTimer->stop();
    m_userId = "";
    m_roomId = "";
    emit disconnected();
}

void WebSocketManager::onTextMessageReceived(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    NetworkMessage networkMsg = NetworkMessage::fromJson(doc.object());
    processMessage(networkMsg);
}

void WebSocketManager::processMessage(const NetworkMessage &message)
{
    switch (message.type) {
    case MT_JoinResponse:
        if (message.data["success"].toBool()) {
            m_userId = message.data["userId"].toString();
            m_roomId = message.data["roomId"].toString();
            emit joinedRoom(m_roomId, m_userId);

            // 处理绘图历史
            if (message.data.contains("drawingHistory")) {
                QJsonArray history = message.data["drawingHistory"].toArray();
                for (const QJsonValue &item : history) {
                    DrawingOperation op = DrawingOperation::fromJson(item.toObject());
                    emit drawingOperationReceived(op);
                }
            }
        }
        break;
    case MT_ClientList:
        // 处理用户列表更新
        break;
    case MT_DrawingOperation:
    {
        DrawingOperation op = DrawingOperation::fromJson(message.data);
        emit drawingOperationReceived(op);
    }
    break;
    case MT_ClearScene:
        emit clearSceneReceived();
        break;
    case MT_UndoRequest:
        emit undoRequestReceived();
        break;
    case MT_RedoRequest:
        emit redoRequestReceived();
        break;
    case MT_ChatMessage:
        emit chatMessageReceived(message.senderId, message.data["message"].toString());
        break;
    case MT_UserRoleChange:
        if (message.data["userId"].toString() == m_userId) {
            m_currentRole = static_cast<UserRole>(message.data["role"].toInt());
        }
        emit userRoleChanged(message.data["userId"].toString(), m_currentRole);
        break;
    case MT_LeaveRequest:
        emit userLeft(message.data["userId"].toString());
        break;
    default:
        break;
    }
}

void WebSocketManager::joinRoom(const QString &roomId, const QString &userName)
{
    m_userName = userName;

    NetworkMessage message;
    message.type = MT_JoinRequest;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    message.data = QJsonObject{
        {"roomId", roomId},
        {"userName", userName}
    };

    sendNetworkMessage(message);
}

void WebSocketManager::sendDrawingOperation(const DrawingOperation &operation)
{
    NetworkMessage message;
    message.type = MT_DrawingOperation;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    message.data = operation.toJson();

    sendNetworkMessage(message);
}

void WebSocketManager::sendNetworkMessage(const NetworkMessage &message)
{
    if (!m_isConnected) return;

    QJsonDocument doc(message.toJson());
    QString data = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    m_webSocket->sendTextMessage(data);
}

void WebSocketManager::sendHeartbeat()
{
    NetworkMessage message;
    message.type = MT_Heartbeat;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    sendNetworkMessage(message);
}

void WebSocketManager::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    emit connectionError(m_webSocket->errorString());
}

// 在 websocketmanager.cpp 中实现：
void WebSocketManager::sendClearScene()
{
    NetworkMessage message;
    message.type = MT_ClearScene;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    sendNetworkMessage(message);
}

void WebSocketManager::sendUndoRequest()
{
    NetworkMessage message;
    message.type = MT_UndoRequest;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    sendNetworkMessage(message);
}

void WebSocketManager::sendRedoRequest()
{
    NetworkMessage message;
    message.type = MT_RedoRequest;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    sendNetworkMessage(message);
}
