#include "websocketserver.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QUuid>

WebSocketServer::WebSocketServer(QObject *parent)
    : QObject(parent)
    , m_webSocketServer(new QWebSocketServer("WhiteboardServer", QWebSocketServer::NonSecureMode, this))
{
    is_join_room = false;
    connect(m_webSocketServer, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
}

WebSocketServer::~WebSocketServer()
{
    stopServer();
}

// 监听来自客户端的连接
bool WebSocketServer::startServer(QString ip, quint16 port)
{
    // 判断是否已经处于监听状态
    if (m_webSocketServer->isListening()) {
        return true;
    }
    // 验证IP地址
    QHostAddress address;
    if (ip.isEmpty()) {
        address = QHostAddress::Any;
    } else if (!address.setAddress(ip)) {
        std::cout<<"无效的IP地址格式"<<std::endl;
        return false;
    }
    if (!m_webSocketServer->listen(address, port)) {
        emit errorOccurred(m_webSocketServer->errorString());
        return false;
    }

    emit serverStarted();
    return true;
}

void WebSocketServer::stopServer()
{
    if (m_webSocketServer->isListening()) {
        m_webSocketServer->close();

        // 断开所有客户端连接，所以这里需要谨慎，确定好是否断开服务器的连接
        for (auto socket : m_clientSockets.values()) {
            socket->close();
            socket->deleteLater();
        }

        m_clients.clear();
        m_clientSockets.clear();
        m_rooms.clear();
        emit serverStopped();
    }
}

void WebSocketServer::onNewConnection()
{
    QWebSocket *socket = m_webSocketServer->nextPendingConnection();
    if (!socket) return;

    // 随机生成的客户端id标识符
    QString clientId = generateClientId();

    ClientInfo clientInfo;
    clientInfo.socket = socket;
    clientInfo.userId = clientId;
    clientInfo.userName = "User_" + clientId.left(4); // 用户id
    clientInfo.role = UR_Editor;
    clientInfo.roomId = "";

    m_clients[socket] = clientInfo;
    m_clientSockets[clientId] = socket;

    connect(socket, &QWebSocket::textMessageReceived, this, &WebSocketServer::onTextMessageReceived);
    connect(socket, &QWebSocket::disconnected, this, &WebSocketServer::onClientDisconnected);

    emit clientConnected(clientId);
}

void WebSocketServer::onClientDisconnected()
{
    // sender：返回指向发送信号的对象的指针。在这里，它用于确定是哪个对象触发了当前的槽函数
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    // 如果客户端那边断开连接的话，判断当前socket是否在对应的容器中保存
    if (socket && m_clients.contains(socket)) {
        QString clientId = m_clients[socket].userId;
        QString roomId = m_clients[socket].roomId;

        // 从房间中移除客户端
        if (!roomId.isEmpty() && m_rooms.contains(roomId)) {
            m_rooms[roomId].clientIds.remove(clientId);

            // 通知其他客户端该用户离开
            NetworkMessage leaveMsg;
            leaveMsg.type = MT_LeaveRequest;
            leaveMsg.senderId = clientId;
            leaveMsg.timestamp = QDateTime::currentSecsSinceEpoch();
            leaveMsg.data = QJsonObject{{"userId", clientId}};

            // 需要将这个客户端离开的消息广播其他的客户端
            broadcastMessage(leaveMsg, clientId);
        }
        // 从容器中移除对应的客户端信息和socket信息
        m_clients.remove(socket);
        m_clientSockets.remove(clientId);
        // 清除该连接
        socket->deleteLater();

        emit clientDisconnected(clientId);
    }
}

void WebSocketServer::onTextMessageReceived(const QString &message)
{

    // 当前和服务端连接的客户端socket
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (!socket || !m_clients.contains(socket)) {
        qWarning() << "收到消息但无法识别发送者";
        return;
    }

    // qDebug() << "收到来自客户端" << m_clients[socket].userId << "的消息";
    // qDebug() << "消息内容:" << message;

    // 解析来自客户端的消息
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    // 解析当前的数据类型并进行消息的处理
    NetworkMessage networkMsg = NetworkMessage::fromJson(doc.object());
    handleClientMessage(socket, networkMsg);
}

void WebSocketServer::handleClientMessage(QWebSocket *socket, const NetworkMessage &message)
{
    if (!m_clients.contains(socket)) return;

    // 得到和当前服务端连接的客户端id以及所在房间号id
    QString clientId = m_clients[socket].userId;
    QString roomId = m_clients[socket].roomId;

    // 创建消息副本并设置发送者ID
    NetworkMessage processedMessage = message;
    processedMessage.senderId = clientId;

    switch (message.type) {
        case MT_JoinRequest:
            // 如果客户端指定了房间号或者没有指定房间号，都会发送请求到服务端这里来调用这个函数，请求加入房间号
            is_join_room = true;
            processJoinRequest(socket, message.data);
            break;

        case MT_CreateRoom:
            processCreateRoomRequest(socket, message.data);
            break;

        case MT_DrawingOperation:
            processDrawingOperation(socket, message.data);
            break;

        case MT_ClearScene:
            processClearScene(socket, message.data);
            break;

        case MT_UndoRequest:
            processUndoRequest(socket, message.data);
            break;

        case MT_RedoRequest:
            processRedoRequest(socket, message.data);
            break;

        case MT_ChatMessage:
            processChatMessage(socket, message.data);
            break;

        case MT_UserRoleChange:
            processUserRoleChange(socket, message.data);
            break;

        case MT_Heartbeat:
            processHeartbeat(socket, message.data);
            break;

        case MT_LeaveRequest:
            processLeaveRequest(socket, message.data);
            break;

        case MT_RoomList:
            processRoomListRequest(socket);
            break;

        default:
            qWarning() << "未知的消息类型:" << message.type;
            sendError(socket, "未知的消息类型");
            break;
    }

    emit messageReceived(message);
}

void WebSocketServer::processJoinRequest(QWebSocket *socket, const QJsonObject &data)
{
    // 获得请求加入的房间号以及对应的客户端用户名
    QString roomId = data["roomId"].toString();
    QString userName = data["userName"].toString(m_clients[socket].userName);

    // std::cout<<"recvived from client message"<<" roomId = "<<roomId.toStdString()<<" userName = "<<userName.toStdString()<<std::endl;

    // 如果客户端那边没有指定房间号就生成一个
    if (roomId.isEmpty()) {
        // 创建新房间
        roomId = generateRoomId();
        RoomInfo room;
        room.roomId = roomId;
        room.roomName = data["roomName"].toString("Default Room");
        m_rooms[roomId] = room;
    } else if (!m_rooms.contains(roomId)) {
        // 房间不存在，发送错误消息给客户端
        NetworkMessage errorMsg;
        errorMsg.type = MT_JoinResponse;
        errorMsg.timestamp = QDateTime::currentSecsSinceEpoch();
        errorMsg.data = QJsonObject{
            {"success", false},
            {"error", "Room not found"}
        };
        sendToClient(socket, errorMsg);
        return;
    }

    // 否则就将当前的客户端加入指定的房间中
    m_clients[socket].userName = userName;
    m_clients[socket].roomId = roomId;
    // 记录当前房间的客户端id
    m_rooms[roomId].clientIds.insert(m_clients[socket].userId);
    // std::cout<<"client ids = "<<m_rooms[roomId].clientIds.size()<<std::endl;

    // 发送加入成功的响应给客户端
    NetworkMessage response;
    response.type = MT_JoinResponse;
    response.timestamp = QDateTime::currentSecsSinceEpoch();
    response.data = QJsonObject{
        {"success", true},
        {"roomId", roomId},
        {"userId", m_clients[socket].userId},
        {"userName", userName},
        {"drawingHistory", m_rooms[roomId].drawingHistory} // 对于刚加入放假的客户端需要同步之前客户端的历史绘图信息
    };

    // 将当前socket客户端加入到房间的消息发送给socket客户端
    sendToClient(socket, response);

    // 广播其他客户端用户有新用户加入
    NetworkMessage notifyMsg;
    notifyMsg.senderId = m_clients[socket].userId;
    notifyMsg.type = MT_ClientList; //注意 这里的消息类型
    notifyMsg.timestamp = QDateTime::currentSecsSinceEpoch();


    QJsonArray clientsArray;
    // 遍历当前房间中的所有客户端id
    for (const QString &clientId : m_rooms[roomId].clientIds) {
        // 如果当前的客户端包含在m_clientSockets中，所以这个客户端是处于连接状态的
        if (m_clientSockets.contains(clientId)) {
            // 获得当前客户端的信息
            ClientInfo info = m_clients[m_clientSockets[clientId]];
            std::cout<<"info.username = "<<info.userName.toStdString()<<std::endl;
            clientsArray.append(QJsonObject{
                {"userId", info.userId},
                {"userName", info.userName},
                {"role", static_cast<int>(info.role)}
            });
        }
    }
    // 当前房间的所有客户端信息，广播给同房间的其他客户端
    notifyMsg.data = QJsonObject{{"clients", clientsArray}};
    broadcastMessage(notifyMsg, m_clients[socket].userId);
}

void WebSocketServer::processDrawingOperation(QWebSocket *socket, const QJsonObject &data)
{
    // 获得当前客户端对应的房间号
    QString roomId = m_clients[socket].roomId;

    // std::cout<<"processDrawingOperation room id = "<<roomId.toStdString()<<std::endl;

    if (roomId.isEmpty()) return;

    // 添加到房间的绘图历史
    DrawingOperation op = DrawingOperation::fromJson(data);
    QJsonObject opJson = op.toJson();

    // 加入对应的历史绘图列表中，便于后面同步加入进来的新客户端
    m_rooms[roomId].drawingHistory.append(opJson);

    // 广播给同一房间的其他用户
    NetworkMessage msg;
    msg.type = MT_DrawingOperation;
    msg.senderId = m_clients[socket].userId;
    msg.timestamp = QDateTime::currentSecsSinceEpoch();
    msg.data = data;

    broadcastMessage(msg, m_clients[socket].userId);
}

void WebSocketServer::broadcastMessage(const NetworkMessage &message, const QString &excludeClientId)
{
    QJsonDocument doc(message.toJson());
    QString data = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    // 获取发送者的房间ID
    QString senderRoomId;
    // 首先判断当前用户id是否存在于集合中
    if (m_clientSockets.contains(message.senderId)) {
        QWebSocket* senderSocket = m_clientSockets[message.senderId];
        // 判断当前的socket是否在m_clients中，为了判断是否处于连接状态
        if (m_clients.contains(senderSocket)) {
            senderRoomId = m_clients[senderSocket].roomId;
        }
    }

    // 如果无法确定发送者的房间，尝试从排除的客户端获取房间ID
    if (senderRoomId.isEmpty() && !excludeClientId.isEmpty() &&
        m_clientSockets.contains(excludeClientId)) {
        QWebSocket* excludeSocket = m_clientSockets[excludeClientId];
        if (m_clients.contains(excludeSocket)) {
            senderRoomId = m_clients[excludeSocket].roomId;
        }
    }

    // qDebug() << "Broadcasting to room = " << senderRoomId << "excluding = " << excludeClientId;
    // qDebug() << "Room clients count = " << m_rooms[senderRoomId].clientIds.size();

    if (!m_rooms.contains(senderRoomId)) {
        qWarning() << "Room not found:" << senderRoomId;
        return;
    }

    if(is_join_room){
        // 如果是新加入客户端的话，所有用户列表都需要更新，包括刚加入进来的客户端
        for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
            if (it->roomId == senderRoomId) {
                std::cout<<"socket user id = "<<(it->userId).toStdString()<<std::endl;
                // it->socket->sendTextMessage(data);
                it.key()->sendTextMessage(data);
            }
        }
        is_join_room = false;
    }else{
        // 这里的广播其实对每一个客户端分别发送相同的消息（除了当前客户端自身以外）
        // 广播给同一房间的所有用户（除了排除的客户端）
        for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
            if (it->userId != excludeClientId && it->roomId == senderRoomId) {
                // std::cout<<"socket user id = "<<(it->userId).toStdString()<<std::endl;
                // it->socket->sendTextMessage(data);
                it.key()->sendTextMessage(data);
            }
        }
    }
}

void WebSocketServer::sendToClient(QWebSocket *socket, const NetworkMessage &message)
{
    QJsonDocument doc(message.toJson());
    QString data = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    socket->sendTextMessage(data);
}

QString WebSocketServer::generateClientId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(6);
}

QString WebSocketServer::generateRoomId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}


void WebSocketServer::processCreateRoomRequest(QWebSocket *socket, const QJsonObject &data)
{
    // 客户端创建，将获取的值转换为QString类型，如果值不存在或转换失败，则使用默认值"New Room"
    QString roomName = data["roomName"].toString("New Room");
    QString userName = data["userName"].toString(m_clients[socket].userName);

    // 如果客户端发送过来的消息中已经生成了房间ID，那么服务端就不需要生成了，否则就要生成room Id
    QString roomId = data["roomId"].toString();
    if(data["roomId"].toString().isEmpty()){
        roomId = generateRoomId();
    }
    std::cout<<"generate room id = "<<roomId.toStdString()<<std::endl;

    // 创建新房间
    RoomInfo room;
    room.roomId = roomId;
    room.roomName = roomName;
    m_rooms[roomId] = room;

    // 将当前的客户端加入到创建的房间中
    m_clients[socket].userName = userName;
    m_clients[socket].roomId = roomId;
    // 记录当前房间有哪些客户端id
    m_rooms[roomId].clientIds.insert(m_clients[socket].userId);

    // 发送创建房间成功的响应
    NetworkMessage response;
    response.type = MT_CreateRoomResponse;
    response.timestamp = QDateTime::currentSecsSinceEpoch();
    response.data = QJsonObject{
        {"success", true},
        {"roomId", roomId},
        {"roomName", roomName},
        {"userId", m_clients[socket].userId}
    };

    sendToClient(socket, response);

    // 通知客户端成功加入房间的响应
    NetworkMessage joinResponse;
    joinResponse.type = MT_JoinResponse;
    joinResponse.timestamp = QDateTime::currentSecsSinceEpoch();
    joinResponse.data = QJsonObject{
        {"success", true},
        {"roomId", roomId},
        {"userId", m_clients[socket].userId},
        {"userName", userName}
    };

    sendToClient(socket, joinResponse);
}

void WebSocketServer::processClearScene(QWebSocket *socket, const QJsonObject &data)
{
    QString roomId = m_clients[socket].roomId;
    if (roomId.isEmpty()) {
        sendError(socket, "未加入任何房间");
        return;
    }

    // 清除房间的绘图历史
    m_rooms[roomId].drawingHistory = QJsonArray();

    // 广播清除场景消息
    NetworkMessage message;
    message.type = MT_ClearScene;
    message.senderId = m_clients[socket].userId;
    message.timestamp = QDateTime::currentSecsSinceEpoch();

    broadcastMessage(message, m_clients[socket].userId);
}

void WebSocketServer::processUndoRequest(QWebSocket *socket, const QJsonObject &data)
{
    QString roomId = m_clients[socket].roomId;
    if (roomId.isEmpty()) {
        sendError(socket, "未加入任何房间");
        return;
    }

    // 获取操作ID（如果有）
    QString operationId = data["operationId"].toString();

    if (!operationId.isEmpty()) {
        // 移除特定操作
        removeOperationFromHistory(roomId, operationId);
    } else if (!m_rooms[roomId].drawingHistory.isEmpty()) {
        // 移除最后一项并记录到撤销栈
        QJsonObject lastOp = m_rooms[roomId].drawingHistory.last().toObject();
        m_rooms[roomId].undoStack.append(lastOp);
        m_rooms[roomId].drawingHistory.removeLast();
    }

    // 广播撤销请求（包含操作信息）
    NetworkMessage message;
    message.type = MT_UndoRequest;
    message.senderId = m_clients[socket].userId;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    message.data = data; // 包含操作信息

    broadcastMessage(message, m_clients[socket].userId);
}

void WebSocketServer::processRedoRequest(QWebSocket *socket, const QJsonObject &data)
{
    QString roomId = m_clients[socket].roomId;
    if (roomId.isEmpty()) {
        sendError(socket, "未加入任何房间");
        return;
    }

    // 从重做栈恢复操作
    if (!m_rooms[roomId].undoStack.isEmpty()) {
        QJsonObject redoneOp = m_rooms[roomId].undoStack.takeLast();
        m_rooms[roomId].drawingHistory.append(redoneOp);

        // 广播重做的具体操作
        NetworkMessage message;
        message.type = MT_DrawingOperation;
        message.senderId = m_clients[socket].userId;
        message.timestamp = QDateTime::currentSecsSinceEpoch();
        message.data = redoneOp;

        broadcastMessage(message);
    }
}

void WebSocketServer::removeOperationFromHistory(const QString &roomId, const QString &operationId)
{
    if (!m_rooms.contains(roomId)) {
        qWarning() << "房间不存在:" << roomId;
        return;
    }

    RoomInfo &room = m_rooms[roomId];

    // 查找并移除指定操作ID的项
    for (int i = room.drawingHistory.size() - 1; i >= 0; --i) {
        QJsonObject operation = room.drawingHistory[i].toObject();
        QString opId = operation["operationId"].toString();

        if (opId == operationId) {
            // 将移除的操作添加到撤销栈
            room.undoStack.append(operation);
            room.drawingHistory.removeAt(i);
            qDebug() << "从绘图历史中移除操作:" << operationId;
            return;
        }
    }

    qWarning() << "未找到操作ID:" << operationId;
}

void WebSocketServer::processUserRoleChange(QWebSocket *socket, const QJsonObject &data)
{
    QString targetUserId = data["userId"].toString();
    int newRole = data["role"].toInt();

    // 检查权限（只有演示者可以修改角色）
    if (m_clients[socket].role != UR_Presenter) {
        sendError(socket, "权限不足");
        return;
    }

    // 查找目标用户
    QWebSocket *targetSocket = m_clientSockets.value(targetUserId);
    if (!targetSocket || !m_clients.contains(targetSocket)) {
        sendError(socket, "用户不存在");
        return;
    }

    // 更新角色
    m_clients[targetSocket].role = static_cast<UserRole>(newRole);

    // 广播角色变更
    NetworkMessage message;
    message.type = MT_UserRoleChange;
    message.senderId = m_clients[socket].userId;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    message.data = QJsonObject{
        {"userId", targetUserId},
        {"role", newRole},
        {"userName", m_clients[targetSocket].userName}
    };

    broadcastMessage(message);
}

void WebSocketServer::processHeartbeat(QWebSocket *socket, const QJsonObject &data)
{
    // 更新客户端最后活动时间
    m_clients[socket].lastActive = QDateTime::currentDateTime();

    // 可选：发送心跳响应
    NetworkMessage response;
    response.type = MT_Heartbeat;
    response.timestamp = QDateTime::currentSecsSinceEpoch();
    response.data = QJsonObject{
        {"status", "alive"},
        {"serverTime", QDateTime::currentDateTime().toString(Qt::ISODate)}
    };

    sendToClient(socket, response);
}

void WebSocketServer::processLeaveRequest(QWebSocket *socket, const QJsonObject &data)
{
    QString roomId = m_clients[socket].roomId;
    if (roomId.isEmpty()) {
        return;
    }

    // 从房间中移除客户端
    m_rooms[roomId].clientIds.remove(m_clients[socket].userId);
    m_clients[socket].roomId = "";

    // 广播离开消息
    NetworkMessage message;
    message.type = MT_LeaveRequest;
    message.senderId = m_clients[socket].userId;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    message.data = QJsonObject{
        {"userId", m_clients[socket].userId},
        {"userName", m_clients[socket].userName}
    };

    broadcastMessage(message, m_clients[socket].userId);

    // 发送离开响应
    NetworkMessage response;
    response.type = MT_LeaveRequest;
    response.timestamp = QDateTime::currentSecsSinceEpoch();
    response.data = QJsonObject{
        {"success", true},
        {"userId", m_clients[socket].userId}
    };

    sendToClient(socket, response);
}

void WebSocketServer::processRoomListRequest(QWebSocket *socket)
{
    // 构建房间列表
    QJsonArray roomsArray;
    for (auto it = m_rooms.begin(); it != m_rooms.end(); ++it) {
        roomsArray.append(QJsonObject{
            {"roomId", it.key()},
            {"roomName", it.value().roomName},
            {"clientCount", it.value().clientIds.size()}
        });
    }

    // 发送房间列表
    NetworkMessage response;
    response.type = MT_RoomList;
    response.timestamp = QDateTime::currentSecsSinceEpoch();
    response.data = QJsonObject{
        {"rooms", roomsArray}
    };

    sendToClient(socket, response);
}

void WebSocketServer::processChatMessage(QWebSocket *socket, const QJsonObject &data)
{
    QString roomId = m_clients[socket].roomId;
    if (roomId.isEmpty()) {
        sendError(socket, "未加入任何房间");
        return;
    }

    QString messageText = data["message"].toString();
    if (messageText.isEmpty()) {
        return;
    }
    // 和当前服务端建立连接的客户端socket用户名
    QString userName = data["userName"].toString(m_clients[socket].userName);

    // 广播聊天消息
    NetworkMessage message;
    message.type = MT_ChatMessage;
    message.senderId = m_clients[socket].userId;
    message.timestamp = QDateTime::currentSecsSinceEpoch();
    message.data = QJsonObject{
        {"message", messageText},
        {"userName", userName},
        {"timestamp", QDateTime::currentDateTime().toString(Qt::ISODate)}
    };

    broadcastMessage(message, m_clients[socket].userId);

    // 记录聊天日志（可选）
    qDebug() << "聊天消息:" << userName << ":" << messageText;
}

void WebSocketServer::sendError(QWebSocket *socket, const QString &errorMessage)
{
    NetworkMessage errorMsg;
    errorMsg.type = MT_RoomError;
    errorMsg.timestamp = QDateTime::currentSecsSinceEpoch();
    errorMsg.data = QJsonObject{
        {"error", errorMessage}
    };

    sendToClient(socket, errorMsg);
}

void WebSocketServer::cleanupInactiveClients()
{
    QDateTime now = QDateTime::currentDateTime();
    for (auto it = m_clients.begin(); it != m_clients.end(); ) {
        // 清理30秒无活动的客户端
        if (it->lastActive.msecsTo(now) > 30000) {
            QString clientId = it->userId;
            QString roomId = it->roomId;

            // 从房间中移除
            if (!roomId.isEmpty() && m_rooms.contains(roomId)) {
                m_rooms[roomId].clientIds.remove(clientId);
            }

            // 关闭连接
            it->socket->close();
            it->socket->deleteLater();

            // 从容器中移除
            m_clientSockets.remove(clientId);
            it = m_clients.erase(it);

            emit clientDisconnected(clientId);
        } else {
            ++it;
        }
    }
}
