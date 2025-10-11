#include "networkmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket)
    , m_isConnected(false)
{
    // 重要：禁用代理，直接连接
    m_webSocket->setProxy(QNetworkProxy::NoProxy);
    connect(m_webSocket, &QWebSocket::connected, this, &NetworkManager::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &NetworkManager::onTextMessageReceived);
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &NetworkManager::onError);
}

NetworkManager::~NetworkManager()
{
    disconnectFromServer();
    delete m_webSocket;
}

// 和服务端建立连接
bool NetworkManager::connectToServer(const QUrl &url)
{
    if (m_isConnected) {
        disconnectFromServer();
    }

    m_serverUrl = url;
    m_webSocket->open(url);
    return true;
}

// 断开连接触发的槽函数
void NetworkManager::disconnectFromServer()
{
    if (m_isConnected) {
        logout();
        m_webSocket->close();
    }
}

// 发送消息
void NetworkManager::sendMessage(const NetworkMessage &message)
{
    if (!m_isConnected) {
        qWarning() << "Not connected to server";
        return;
    }

    QByteArray jsonData = message.toJson();
    m_webSocket->sendTextMessage(QString::fromUtf8(jsonData));
}

// 发送登录消息
void NetworkManager::login(const QString &username)
{
    m_username = username;
    NetworkMessage message = createLoginMessage(username);
    sendMessage(message);
}

// 发送退出登录的 消息
void NetworkManager::logout()
{
    if (!m_username.isEmpty()) {
        NetworkMessage message;
        message.type = MessageType::Logout;
        QJsonObject data;
        data["username"] = m_username;
        message.data = data;
        sendMessage(message);
    }
}

// 发送棋子落子的消息
void NetworkManager::sendMove(int x, int y, PieceType piece)
{
    NetworkMessage message = createMoveMessage(x, y, piece);
    sendMessage(message);
}

// 发送游戏开始的消息
void NetworkManager::sendGameStart(const QString &opponent, PieceType myPieceType)
{
    NetworkMessage message;
    message.type = MessageType::GameStart;
    QJsonObject data;
    data["opponent"] = opponent;
    data["myPieceType"] = static_cast<int>(myPieceType); // 添加棋子类型
    data["opponentPieceType"] = static_cast<int>(myPieceType == PieceType::Black ? PieceType::White : PieceType::Black);
    message.data = data;
    sendMessage(message);
    // NetworkMessage message = createGameStartMessage(opponent);
    // sendMessage(message);
}

// 发送游戏结束的消息
void NetworkManager::sendGameEnd(const QString &winner)
{
    NetworkMessage message = createGameEndMessage(winner);
    sendMessage(message);
}

// 发送聊天消息
void NetworkManager::sendChatMessage(const QString& sender, const QString &message)
{
    NetworkMessage msg = createChatMessage(sender, message);
    sendMessage(msg);
}

// 判断是否建立的连接
bool NetworkManager::isConnected() const
{
    return m_isConnected;
}

QString NetworkManager::getUsername() const
{
    return m_username;
}

// 发送建立连接的消息给client
void NetworkManager::onConnected()
{
    m_isConnected = true;
    emit connected();
    qDebug() << "Connected to server:" << m_serverUrl.toString();
}

//发送断开连接的消息
void NetworkManager::onDisconnected()
{
    m_isConnected = false;
    emit disconnected();
    qDebug() << "Disconnected from server";
}

void NetworkManager::onTextMessageReceived(const QString &message)
{
    QByteArray jsonData = message.toUtf8();
    NetworkMessage networkMessage = NetworkMessage::fromJson(jsonData);

    if (networkMessage.type == MessageType::Invalid) {
        qWarning() << "Invalid message received";
        return;
    }

    emit messageReceived(networkMessage);

    // 分发特定类型的消息
    switch (networkMessage.type) {
        case MessageType::UserListResponse: {
            QStringList users;
            QJsonArray usersArray = networkMessage.data["users"].toArray();
            for (const QJsonValue &userValue : usersArray) {
                users.append(userValue.toString());
            }
            std::cout<<"MessageType::UserListResponse"<<" user number = "<<users.size()<<std::endl;
            emit userListReceived(users);
            break;
        }
        case MessageType::Login: {
            QString username = networkMessage.data["username"].toString();
            QString status = networkMessage.data["status"].toString();
            QString action = networkMessage.data["action"].toString();

            if (!action.isEmpty()) {
                // 这是广播的用户加入/退出通知
                emit userStatusChanged(username, action);
            } else if (status == "success") {
                // 这是登录成功的响应
                emit loginSuccess(username);
            } else {
                // 登录失败
                emit loginFailed(networkMessage.data["error"].toString());
            }
            break;
        }

        case MessageType::Logout: {
            QString username = networkMessage.data["username"].toString();
            emit logoutReceived(username);
            break;
            }
        case MessageType::Move: {
            int x = networkMessage.data["x"].toInt();
            int y = networkMessage.data["y"].toInt();
            PieceType piece = static_cast<PieceType>(networkMessage.data["piece"].toInt());
            emit moveReceived(x, y, piece);
            break;
        }
        case MessageType::GameStart: {
            QString opponent = networkMessage.data["opponent"].toString();
            PieceType myPieceType = static_cast<PieceType>(networkMessage.data["myPieceType"].toInt());
            PieceType opponentPieceType = static_cast<PieceType>(networkMessage.data["opponentPieceType"].toInt());
            emit gameStartReceived(opponent, myPieceType, opponentPieceType);
            break;
        }
        case MessageType::GameEnd: {
            QString winner = networkMessage.data["winner"].toString();
            emit gameEndReceived(winner);
            break;
        }
        case MessageType::Chat: {
            QString chatMsg = networkMessage.data["message"].toString();
            std::cout<<"MessageType::Chat"<<" chat message"<<std::endl;
            emit chatMessageReceived(networkMessage.data["username"].toString(), chatMsg);
            break;
        }
        case MessageType::Error: {
            QString error = networkMessage.data["error"].toString();
            emit errorOccurred(error);
            break;
        }
        case MessageType::RoomCreated: {
            // 获得房间的ID和房间的名称
            QString roomId = networkMessage.data["roomId"].toString();
            QString roomName = networkMessage.data["roomName"].toString();
            std::cout<<"MessageType::RoomCreated"<<" room id = "<<roomId.toStdString()<<std::endl;
            // 通过消息发送给client，client去处理
            emit roomCreated(roomId, roomName);
            break;
        }

        case MessageType::RoomJoined: {
            // 请求加入的房间ID和房间名称
            QString roomId = networkMessage.data["roomId"].toString();
            QString roomName = networkMessage.data["roomName"].toString();

            std::cout<<"MessageType::RoomJoined"<<" room id = "<<roomId.toStdString()<<std::endl;

            // 获得该房间下面的所有成员信息
            QStringList members;
            QJsonArray membersArray = networkMessage.data["members"].toArray();
            for (const QJsonValue &memberValue : membersArray) {
                members.append(memberValue.toString());
                std::cout<<"MessageType::RoomJoined"<<" room id = "<<memberValue.toString().toStdString()<<std::endl;
            }
            // 请求加入房间的消息发送给client
            emit roomJoined(roomId, roomName, members);
            break;
        }

        case MessageType::RoomLeft: {
            // 离开房间roomID
            QString roomId = networkMessage.data["roomId"].toString();
            emit roomLeft(roomId);
            break;
        }

        case MessageType::RoomListResponse: {
            // 房间信息
            QJsonArray rooms = networkMessage.data["rooms"].toArray();
            emit roomListReceived(rooms);
            break;
        }
        case MessageType::StatsUpdate: {
            QString username = networkMessage.data["username"].toString();
            int score = networkMessage.data["score"].toInt();
            int wins = networkMessage.data["wins"].toInt();
            int losses = networkMessage.data["losses"].toInt();
            int draws = networkMessage.data["draws"].toInt();
            int streak = networkMessage.data["streak"].toInt();

            // 发射新的信号
            emit statsUpdated(username, score, wins, losses, draws, streak);
            break;
        }
        case MessageType::ResumeStart:{
            QString username = networkMessage.data["username"].toString();

            emit sendResumeStart();
            break;
        }
        case MessageType::GIVE_UP: {
            QString username = networkMessage.data["username"].toString();

            emit sendGiveUpsignal(username);
            break;
        }
        case MessageType::PEACE: {
            QString username = networkMessage.data["username"].toString();
            emit sendPeaceSignal();
            break;
        }
        default:
            break;
    }
}

void NetworkManager::onError(QAbstractSocket::SocketError error)
{
    // 将错误信息发送给client
    QString errorMsg = m_webSocket->errorString();
    emit errorOccurred(errorMsg);
    qWarning() << "WebSocket error:" << errorMsg;
}

// client中会调用这个函数创建房间的消息发送服务端
void NetworkManager::createRoom(const QString &roomName, const QString &roomId)
{
    // 首先判断客户端和服务端是否处于连接状态，
    if (!m_isConnected) {
        qWarning() << "Not connected to server, cannot create room";
        emit roomError("未连接到服务器");
        return;
    }

    NetworkMessage message;
    message.type = MessageType::CreateRoom;
    QJsonObject data;
    data["roomName"] = roomName;
    data["roomId"] = roomId;
    message.data = data;

    sendMessage(message);

    qDebug() << "Sent create room request:" << roomName << "ID:" << roomId;
}
// client中会调用这个函数加入房间的消息发送服务端
void NetworkManager::joinRoom(const QString &roomId)
{
    if (!m_isConnected) {
        qWarning() << "Not connected to server, cannot join room";
        emit roomError("未连接到服务器");
        return;
    }

    NetworkMessage message;
    message.type = MessageType::JoinRoom;
    QJsonObject data;
    data["roomId"] = roomId;
    message.data = data;

    sendMessage(message);

    qDebug() << "Sent join room request, room ID:" << roomId;
}
// client中会调用这个函数离开房间的消息发送服务端
void NetworkManager::leaveRoom(const QString &roomId)
{
    if (!m_isConnected) {
        qWarning() << "Not connected to server, cannot leave room";
        emit roomError("未连接到服务器");
        return;
    }

    NetworkMessage message;
    message.type = MessageType::LeaveRoom;
    QJsonObject data;
    data["roomId"] = roomId;
    message.data = data;

    sendMessage(message);

    qDebug() << "Sent leave room request, room ID:" << roomId;
}

void NetworkManager::requestRoomList()
{
    if (!m_isConnected) {
        qWarning() << "Not connected to server, cannot request room list";
        return;
    }

    NetworkMessage message;
    message.type = MessageType::RoomList;
    QJsonObject data;
    message.data = data;

    sendMessage(message);

    qDebug() << "Sent room list request";
}

void NetworkManager::requestUserList()
{
    if (!m_isConnected) {
        qWarning() << "Not connected to server, cannot request user list";
        return;
    }

    NetworkMessage message;
    message.type = MessageType::UserList; // 需要定义 UserList 消息类型
    QJsonObject data;
    message.data = data;

    sendMessage(message);
    qDebug() << "Sent user list request";
}

// 处理服务器发送的用户列表消息
void NetworkManager::handleUserListMessage(const QJsonArray &users)
{
    QStringList userList;
    for (const QJsonValue &user : users) {
        userList.append(user.toString());
    }
    emit userListReceived(userList);
}

void NetworkManager::sendStatsUpdate(const QString &username, int score,
                                     int wins, int losses, int draws, int streak) {
    NetworkMessage message;
    message.type = MessageType::StatsUpdate;
    QJsonObject data;
    data["username"] = username;
    data["score"] = score;
    data["wins"] = wins;
    data["losses"] = losses;
    data["draws"] = draws;
    data["streak"] = streak;
    message.data = data;

    sendMessage(message);
}

void NetworkManager::sendResumeMessage(const QString& roomId){
    NetworkMessage message;
    message.type = MessageType::ResumeStart;
    QJsonObject data;
    data["username"] = "all";
    data["roomId"] = roomId;
    message.data = data;

    sendMessage(message);
}

void NetworkManager::sendGiveUpMessage(const QString& username, const QString& roomId){
    NetworkMessage message;
    message.type = MessageType::GIVE_UP;
    QJsonObject data;
    data["username"] = username;
    data["roomId"] = roomId;
    message.data = data;

    sendMessage(message);
}

void NetworkManager::sendPeaceMessage(){
    NetworkMessage message;
    message.type = MessageType::PEACE;
    QJsonObject data;
    data["username"] = "All";
    message.data = data;

    sendMessage(message);
}
