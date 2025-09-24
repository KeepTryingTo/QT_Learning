#ifndef NETWORKPROTOCOL_H
#define NETWORKPROTOCOL_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainterPath>

// 确保包含所有消息类型
enum MessageType {
    MT_Unknown = 0,
    MT_JoinRequest,         // 加入房间请求
    MT_JoinResponse,        // 加入房间响应
    MT_CreateRoom,          // 创建房间请求
    MT_CreateRoomResponse,  // 创建房间响应
    MT_ClientList,          // 客户端列表
    MT_DrawingOperation,    // 绘图操作
    MT_ClearScene,          // 清除场景
    MT_UndoRequest,         // 撤销请求
    MT_RedoRequest,         // 重做请求
    MT_ChatMessage,         // 聊天消息
    MT_UserRoleChange,      // 用户角色变更
    MT_Heartbeat,           // 心跳检测
    MT_LeaveRequest,        // 离开请求
    MT_RoomList,            // 房间列表请求
    MT_RoomError            // 房间错误
};

// 确保枚举值正确
enum DrawingOperationType {
    DOT_BeginStroke = 0,    // 开始笔画
    DOT_AddPoint = 1,       // 添加点
    DOT_EndStroke = 2,      // 结束笔画
    DOT_DrawLine = 3,       // 绘制直线
    DOT_DrawRectangle = 4,  // 绘制矩形
    DOT_DrawEllipse = 5,    // 绘制椭圆
    DOT_AddText = 6,        // 添加文本
    DOT_Erase = 7,           // 擦除
    DOT_Undo = 8,
    DOT_Redo = 9
};

// 用户角色枚举
enum UserRole {
    UR_Viewer = 0,          // 观察者（只能查看）
    UR_Editor = 1,          // 编辑者（可以绘图）
    UR_Presenter = 2        // 演示者（可以控制其他人）
};

// 网络消息结构体
struct NetworkMessage
{
    MessageType type;
    QJsonObject data;
    QString senderId;
    qint64 timestamp;

    QJsonObject toJson() const;
    static NetworkMessage fromJson(const QJsonObject &json);
};

// 绘图操作数据结构
struct DrawingOperation
{
    DrawingOperationType opType;
    QVariantMap data;
    QString operationId;

    QJsonObject toJson() const;
    static DrawingOperation fromJson(const QJsonObject &json);
};


#endif // NETWORKPROTOCOL_H
