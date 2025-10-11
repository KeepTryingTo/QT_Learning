#include "networkprotocol.h"
#include <QJsonDocument>
#include <QJsonObject>

QByteArray NetworkMessage::toJson() const
{
    QJsonObject json;
    json["type"] = static_cast<int>(type);
    json["data"] = data;

    QJsonDocument doc(json);
    return doc.toJson(QJsonDocument::Compact);
}

NetworkMessage NetworkMessage::fromJson(const QByteArray &json)
{
    NetworkMessage message;

    QJsonDocument doc = QJsonDocument::fromJson(json);
    if (doc.isNull()) {
        message.type = MessageType::Invalid;
        return message;
    }

    QJsonObject obj = doc.object();
    message.type = static_cast<MessageType>(obj["type"].toInt());
    message.data = obj["data"].toObject();

    return message;
}

NetworkMessage createLoginMessage(const QString &username)
{
    NetworkMessage message;
    message.type = MessageType::Login;
    QJsonObject data;
    data["username"] = username;
    message.data = data;
    return message;
}

NetworkMessage createMoveMessage(int x, int y, PieceType piece)
{
    NetworkMessage message;
    message.type = MessageType::Move;
    QJsonObject data;
    data["x"] = x;
    data["y"] = y;
    data["piece"] = static_cast<int>(piece);
    message.data = data;
    return message;
}

NetworkMessage createGameStartMessage(const QString &opponent)
{
    NetworkMessage message;
    message.type = MessageType::GameStart;
    QJsonObject data;
    data["opponent"] = opponent;
    message.data = data;
    return message;
}

NetworkMessage createGameEndMessage(const QString &winner)
{
    NetworkMessage message;
    message.type = MessageType::GameEnd;
    QJsonObject data;
    data["winner"] = winner;
    message.data = data;
    return message;
}

NetworkMessage createChatMessage(const QString &message)
{
    NetworkMessage msg;
    msg.type = MessageType::Chat;
    QJsonObject data;
    data["message"] = message;
    msg.data = data;
    return msg;
}
// 创建房间请求
NetworkMessage createCreateRoomMessage(const QString &roomName, const QString &roomId)
{
    NetworkMessage message;
    message.type = MessageType::CreateRoom;
    QJsonObject data;
    data["roomName"] = roomName;
    data["roomId"] = roomId;
    message.data = data;
    return message;
}

// 加入房间请求
NetworkMessage createJoinRoomMessage(const QString &roomId)
{
    NetworkMessage message;
    message.type = MessageType::JoinRoom;
    QJsonObject data;
    data["roomId"] = roomId;
    message.data = data;
    return message;
}

// 离开房间请求
NetworkMessage createLeaveRoomMessage(const QString &roomId)
{
    NetworkMessage message;
    message.type = MessageType::LeaveRoom;
    QJsonObject data;
    data["roomId"] = roomId;
    message.data = data;
    return message;
}

// 房间列表响应
NetworkMessage createRoomListMessage(const QJsonArray &rooms)
{
    NetworkMessage message;
    message.type = MessageType::RoomList;
    QJsonObject data;
    data["rooms"] = rooms;
    message.data = data;
    return message;
}

// 房间创建成功响应
NetworkMessage createRoomCreatedMessage(const QString &roomId, const QString &roomName)
{
    NetworkMessage message;
    message.type = MessageType::RoomCreated;
    QJsonObject data;
    data["roomId"] = roomId;
    data["roomName"] = roomName;
    message.data = data;
    return message;
}

// 加入房间成功响应
NetworkMessage createRoomJoinedMessage(const QString &roomId, const QString &roomName, const QStringList &members)
{
    NetworkMessage message;
    message.type = MessageType::RoomJoined;
    QJsonObject data;
    data["roomId"] = roomId;
    data["roomName"] = roomName;

    QJsonArray membersArray;
    for (const QString &member : members) {
        membersArray.append(member);
    }
    data["members"] = membersArray;

    message.data = data;
    return message;
}

// networkprotocol.cpp
NetworkMessage createStatsUpdateMessage(const QString &username, int score,
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
    return message;
}
