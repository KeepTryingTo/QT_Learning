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
    // 重要：禁用代理，直接连接
    m_webSocket->setProxy(QNetworkProxy::NoProxy);

    connect(m_webSocket, &QWebSocket::connected, this, &WebSocketManager::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &WebSocketManager::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketManager::onTextMessageReceived);
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, &WebSocketManager::onErrorOccurred);

    // 30秒自动检查一次连接是否还在继续
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
    // 连接服务器
    m_webSocket->open(serverUrl);
    return true;
}

void WebSocketManager::disconnectFromServer()
{
    // 断开和服务器的连接
    if (m_isConnected) {
        // leaveRoom();
        m_webSocket->close();
        m_heartbeatTimer->stop();
    }
}

void WebSocketManager::onConnected()
{
    // 修改连接状态和启动定时器
    m_isConnected = true;
    m_heartbeatTimer->start();
    // 发送连接成功的消息给客户端client
    emit connected();
}

void WebSocketManager::onDisconnected()
{
    m_isConnected = false;
    m_webSocket->close();
    m_heartbeatTimer->stop();
    m_userId = "";
    m_roomId = "";
    emit disconnected();
}

void WebSocketManager::onTextMessageReceived(const QString &message)
{
    // 接收来自服务端的消息
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    // 解析消息并进行处理
    NetworkMessage networkMsg = NetworkMessage::fromJson(doc.object());
    processMessage(networkMsg);
}

void WebSocketManager::processMessage(const NetworkMessage &message)
{
    // 根据消息类型来进行处理
    switch (message.type) {
        case MT_JoinResponse: // 加入房间消息
            std::cout<<"join room = "<<message.data["roomId"].toString().toStdString()<<std::endl;
            if (message.data["success"].toBool()) {
                // 成功加入指定房间号
                m_userId = message.data["userId"].toString();
                m_roomId = message.data["roomId"].toString();
                // emit joinedRoom(m_roomId, m_userId);
                QString userName = message.data["userName"].toString();
                int role = message.data["role"].toInt();
                // 发送信号通知有用户加入
                emit userJoined(m_userId, userName, role);

                // 如果包含历史绘图，就需要处理历史绘图在当前布局中
                if (message.data.contains("drawingHistory")) {
                    QJsonArray history = message.data["drawingHistory"].toArray();
                    // 解析绘图并将绘图结果绘制当前客户端布局中
                    for (const QJsonValue &item : history) {
                        DrawingOperation op = DrawingOperation::fromJson(item.toObject());
                        emit drawingOperationReceived(op);
                    }
                }
            }else {
                QString error = message.data["error"].toString();
                emit roomError(error);
            }
            break;
        case MT_ClientList:
            // 处理用户列表更新
            processClientList(message.data);
            break;
        case MT_DrawingOperation:
        {
            DrawingOperation op = DrawingOperation::fromJson(message.data);
            // qDebug() << "绘图操作类型:" << op.opType;
            // qDebug() << "操作数据键值:" << op.data.keys();

            if (op.data.contains("path")) {
                qDebug() << "包含路径数据";
            }

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
        case MT_ChatMessage:// 发送聊天信息
            emit chatMessageReceived(message.data["userName"].toString(), message.data["message"].toString());
            break;
        case MT_UserRoleChange:
            // 同步改变用于角色信息
            if (message.data["userId"].toString() == m_userId) {
                m_currentRole = static_cast<UserRole>(message.data["role"].toInt());
            }
            emit userRoleChanged(message.data["userId"].toString(), m_currentRole);
            break;
        case MT_LeaveRequest:
            // 用户离线信息
            emit userLeft(message.data["userId"].toString());
            break;
        case MT_CreateRoomResponse:
            // 创建新的房间信息
            {
                bool success = message.data["success"].toBool();
                QString roomId = message.data["roomId"].toString();
                QString roomName = message.data["roomName"].toString();
                m_userId = message.data["userId"].toString();

                if (success) {
                    std::cout<<"create room = "<<roomId.toStdString()<<std::endl;
                    m_currentRoomId = roomId;
                    m_currentRoomName = roomName;
                    emit roomCreated(roomId, roomName);
                } else {
                    QString error = message.data["error"].toString();
                    emit roomError(error);
                }
            }
            break;
        case MT_RoomList:
            // 列出当前房间列表信息
            {
                m_availableRooms.clear();
                QJsonArray roomsArray = message.data["rooms"].toArray();
                for (const QJsonValue &roomValue : roomsArray) {
                    QJsonObject roomObj = roomValue.toObject();
                    QString roomId = roomObj["roomId"].toString();
                    QString roomName = roomObj["roomName"].toString();
                    m_availableRooms.insert(roomId, roomName);
                }
                emit roomListReceived(getAvailableRooms());
            }
            break;

        case MT_RoomError:
            emit roomError(message.data["error"].toString());
            break;
        default:
            break;
    }
}

// 处理来自服务端发送的客户端信息id
void WebSocketManager::processClientList(const QJsonObject &data)
{
    try {
        // 清空当前用户列表
        m_roomClients.clear();

        // 解析客户端列表
        if (data.contains("clients") && data["clients"].isArray()) {
            // 获得当前所有客户端信息
            QJsonArray clientsArray = data["clients"].toArray();

            QList<QJsonObject> clientList;

            // 遍历每一个客户端信息
            for (const QJsonValue &clientValue : clientsArray) {
                if (clientValue.isObject()) {
                    QJsonObject clientObj = clientValue.toObject();

                    QString userId = clientObj["userId"].toString();
                    QString userName = clientObj["userName"].toString();
                    int role = clientObj["role"].toInt();

                    // 存储到本地映射
                    m_roomClients[userId] = clientObj;

                    // 添加到返回列表
                    clientList.append(clientObj);

                    qDebug() << "房间用户:" << userName << "(" << userId << "), 角色:" << role;
                }
            }

            // 发出信号通知客户端列表更新
            emit clientListReceived(clientList);
        }
    } catch (const std::exception &e) {
        qWarning() << "处理客户端列表错误:" << e.what();
    }
}

// 不管下面是加入房间，清除绘图，绘图操作，创建房间等操作都要同步到其他的在线客户端用户
void WebSocketManager::joinRoom(const QString &roomId, const QString &userName)
{
    m_currentRoomId = roomId;
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
    // qDebug() << "准备发送绘图操作，操作类型:" << operation.opType;
    // qDebug() << "操作数据:" << operation.data;
    // qDebug() << "当前连接状态:" << m_webSocket->state();
    // qDebug() << "是否已连接:" << m_webSocket->isValid();

    NetworkMessage message;
    // 当前是绘图操作
    message.type = MT_DrawingOperation;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    // 转换为指定的格式之后再发送数据（包括当前具体操作类型，比如是画笔，矩形等）
    message.data = operation.toJson();

    sendNetworkMessage(message);
}

// 发送聊天消息
void WebSocketManager::sendChatMessage(const QString &messageText)
{
    if (messageText.trimmed().isEmpty()) {
        return;
    }

    NetworkMessage message;
    message.type = MT_ChatMessage;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    message.senderId = m_userId;
    message.data = QJsonObject{
        {"message", messageText},
        {"userName", m_userName},
        {"timestamp", QDateTime::currentDateTime().toString(Qt::ISODate)}
    };

    sendNetworkMessage(message);
}

void WebSocketManager::sendNetworkMessage(const NetworkMessage &message)
{
    if (!m_isConnected) {
        qWarning() << "网络未连接，无法发送消息";
        return;
    }

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

QString WebSocketManager::createRoom(const QString &roomName)
{
    if (!m_webSocket || !m_webSocket->isValid()) {
        emit roomError("未连接到服务器");
        return QString();
    }

    // 如果在创建房间的时候，没有指定对应的房间号，就随机的生成唯一的房间ID(生成简短的唯一标识符（8字符）)
    QString roomId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);

    // 创建房间创建消息
    NetworkMessage message;
    message.type = MessageType::MT_CreateRoom;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    message.data = QJsonObject{
        {"roomId", roomId},
        {"roomName", roomName},
        {"userName", m_userName}
    };

    // 发送消息
    sendNetworkMessage(message);

    // 等待服务器响应（这里需要异步处理）
    // 实际实现中，服务器会返回确认消息

    return roomId;
}


void WebSocketManager::leaveRoom()
{
    if (m_currentRoomId.isEmpty()) {
        return;
    }

    NetworkMessage message;
    message.type = MT_LeaveRequest;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    message.data = QJsonObject{
        {"roomId", m_currentRoomId}
    };

    sendNetworkMessage(message);
    // 清除房间id，房间名称
    m_currentRoomId.clear();
    m_currentRoomName.clear();
    emit roomLeft(m_currentRoomId);
}

QString WebSocketManager::getUserId(){
    return m_userId;
}

QString WebSocketManager::getCurrentRoomId()
{
    return m_currentRoomId;
}

QString WebSocketManager::getCurrentRoomName() const
{
    return m_currentRoomName;
}
// 遍历所有可利用的房间ID
QList<QPair<QString, QString>> WebSocketManager::getAvailableRooms() const
{
    QList<QPair<QString, QString>> rooms;
    for (auto it = m_availableRooms.begin(); it != m_availableRooms.end(); ++it) {
        rooms.append(qMakePair(it.key(), it.value()));
    }
    return rooms;
}

