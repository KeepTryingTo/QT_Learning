#include "networkprotocol.h"
#include <QDateTime>

QJsonObject NetworkMessage::toJson() const
{
    QJsonObject json;
    json["type"] = static_cast<int>(type);
    json["data"] = data;
    json["senderId"] = senderId;
    json["timestamp"] = timestamp;
    return json;
}

NetworkMessage NetworkMessage::fromJson(const QJsonObject &json)
{
    NetworkMessage msg;
    msg.type = static_cast<MessageType>(json["type"].toInt());
    msg.data = json["data"].toObject();
    msg.senderId = json["senderId"].toString();
    msg.timestamp = json["timestamp"].toInt();
    return msg;
}

// 添加转换方法
QJsonObject DrawingOperation::toJson() const
{
    QJsonObject json;
    json["opType"] = static_cast<int>(opType);

    QJsonObject dataJson;
    for (auto it = data.begin(); it != data.end(); ++it) {
        // 处理特殊类型
        if (it.value().canConvert<QPainterPath>()) {
            QPainterPath path = it.value().value<QPainterPath>();
            // 将路径转换为可序列化的格式
            QJsonArray pathArray;
            for (int i = 0; i < path.elementCount(); ++i) {
                QJsonObject point;
                point["x"] = path.elementAt(i).x;
                point["y"] = path.elementAt(i).y;
                point["type"] = path.elementAt(i).type;
                pathArray.append(point);
            }
            dataJson[it.key()] = pathArray;
        } else {
            dataJson[it.key()] = QJsonValue::fromVariant(it.value());
        }
    }
    json["data"] = dataJson;

    return json;
}

DrawingOperation DrawingOperation::fromJson(const QJsonObject &json)
{
    DrawingOperation op;
    op.opType = static_cast<DrawingOperationType>(json["opType"].toInt());

    QJsonObject dataJson = json["data"].toObject();
    for (auto it = dataJson.begin(); it != dataJson.end(); ++it) {
        op.data[it.key()] = it.value().toVariant();
    }

    return op;
}
