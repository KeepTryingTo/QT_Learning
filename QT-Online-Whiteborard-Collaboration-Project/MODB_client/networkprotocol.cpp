#include "networkprotocol.h"
#include <QDateTime>

// 使用JSON格式保存数据并返回
QJsonObject NetworkMessage::toJson() const
{
    QJsonObject json;
    json["type"] = static_cast<int>(type);
    json["data"] = data;
    json["senderId"] = senderId;
    json["timestamp"] = timestamp;
    return json;
}

// 解析JSON格式数据
NetworkMessage NetworkMessage::fromJson(const QJsonObject &json)
{
    NetworkMessage msg;
    msg.type = static_cast<MessageType>(json["type"].toInt());
    msg.data = json["data"].toObject();
    msg.senderId = json["senderId"].toString();
    msg.timestamp = json["timestamp"].toInt();
    return msg;
}

QJsonObject DrawingOperation::toJson() const
{
    QJsonObject json;
    json["opType"] = static_cast<int>(opType);

    QJsonObject dataJson;
    for (auto it = data.begin(); it != data.end(); ++it) {
        // 处理特殊类型 - 路径
        if (it.value().canConvert<QPainterPath>()) {
            QPainterPath path = it.value().value<QPainterPath>();

            // 将路径转换为可序列化的格式
            QJsonArray pathArray;
            for (int i = 0; i < path.elementCount(); ++i) {
                QJsonObject point;
                const QPainterPath::Element &element = path.elementAt(i);
                point["x"] = element.x;
                point["y"] = element.y;
                point["type"] = element.type; // 0=MoveTo移动到新位置, 1=LineTo直线到新位置, 2=CurveTo贝塞尔曲线的控制点
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
        // 处理路径反序列化
        if (it.value().isArray() && (it.key() == "path" || it.key().contains("Path"))) {
            QPainterPath path;
            QJsonArray pathArray = it.value().toArray();

            for (const QJsonValue &pointValue : pathArray) {
                QJsonObject pointObj = pointValue.toObject();
                qreal x = pointObj["x"].toDouble();
                qreal y = pointObj["y"].toDouble();
                int type = pointObj["type"].toInt();

                if (type == QPainterPath::MoveToElement) {
                    path.moveTo(x, y);
                } else if (type == QPainterPath::LineToElement) {
                    path.lineTo(x, y);
                } else if (type == QPainterPath::CurveToElement) {
                    // 处理曲线点（需要三个点）
                    // 这里简化处理，实际需要更复杂的逻辑
                    path.lineTo(x, y);
                }
            }
            op.data[it.key()] = QVariant::fromValue(path);
        } else {
            op.data[it.key()] = it.value().toVariant();
        }
    }

    return op;
}
