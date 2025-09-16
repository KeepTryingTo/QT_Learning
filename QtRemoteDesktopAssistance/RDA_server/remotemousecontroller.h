#ifndef REMOTEMOUSECONTROLLER_H
#define REMOTEMOUSECONTROLLER_H

#include <QObject>
#include <QPoint>
#include <QtGlobal>

class RemoteMouseController : public QObject
{
    Q_OBJECT
public:
    explicit RemoteMouseController(QObject *parent = nullptr);

    // 处理接收到的鼠标事件
    void processMouseEvent(const QPoint &position, Qt::MouseButton button, const QString &action);

    // 检查是否具有模拟鼠标事件的权限
    bool hasMouseControlPermission() const;

signals:
    // 当鼠标事件被成功模拟时发出
    void mouseEventSimulated(const QString &eventInfo);
    // 当模拟鼠标事件失败时发出
    void mouseEventFailed(const QString &error);

private:
    // 平台特定的鼠标事件模拟实现
    void simulateMouseEvent(const QPoint &position, Qt::MouseButton button, const QString &action);

    // 检查权限的私有方法
    bool checkPermissions() const;
};

#endif // REMOTEMOUSECONTROLLER_H
