#pragma once
#ifndef NETWORKPROTOCOL_H
#define NETWORKPROTOCOL_H

#include <QString>
#include <QJsonObject>
#include <QPoint>
#include <QJsonArray>

#include "piecetype.h"


enum class MessageType {
    Invalid = 0,
    Login,          // 登录
    Logout,         // 登出
    GameStart,      // 游戏开始
    Move,           // 走棋
    GameEnd,        // 游戏结束
    Chat,           // 聊天
    Error,           // 错误
    CreateRoom,    // 新增：创建房间
    JoinRoom,      // 新增：加入房间
    LeaveRoom,     // 新增：离开房间
    RoomList,      // 新增：房间列表
    RoomCreated,   // 新增：房间创建成功
    RoomJoined,    // 新增：加入房间成功
    RoomLeft,      // 新增：离开房间成功
    RoomListResponse,
    UserList,
    UserListResponse,
    StatsUpdate,
    ResumeStart,
    GIVE_UP,
    PEACE
};

struct NetworkMessage {
    MessageType type;
    QJsonObject data;

    QByteArray toJson() const;
    static NetworkMessage fromJson(const QByteArray &json);
};

// 消息工厂函数
NetworkMessage createLoginMessage(const QString &username);
NetworkMessage createMoveMessage(int x, int y, PieceType piece);
NetworkMessage createGameStartMessage(const QString &opponent);
NetworkMessage createGameEndMessage(const QString &winner);
NetworkMessage createChatMessage(const QString& sender, const QString &message);

NetworkMessage createRoomJoinedMessage(const QString &roomId, const QString &roomName, const QStringList &members);
NetworkMessage createRoomCreatedMessage(const QString &roomId, const QString &roomName);
NetworkMessage createRoomListMessage(const QJsonArray &rooms);
NetworkMessage createCreateRoomMessage(const QString &roomName, const QString &roomId);
// 加入房间请求
NetworkMessage createJoinRoomMessage(const QString &roomId);
// 离开房间请求
NetworkMessage createLeaveRoomMessage(const QString &roomId);

// 添加创建统计更新消息的函数
NetworkMessage createStatsUpdateMessage(const QString &username, int score,
                                        int wins, int losses, int draws, int streak);

#endif // NETWORKPROTOCOL_H
