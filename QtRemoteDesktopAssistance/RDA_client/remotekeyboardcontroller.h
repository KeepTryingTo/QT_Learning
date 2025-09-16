#ifndef REMOTEKEYBOARDCONTROLLER_H
#define REMOTEKEYBOARDCONTROLLER_H

#include <QObject>
#include <QKeyEvent>
#include <QtGlobal>

class RemoteKeyboardController : public QObject
{
    Q_OBJECT
public:
    explicit RemoteKeyboardController(QObject *parent = nullptr);

    // 处理接收到的键盘事件
    void processKeyEvent(int key, Qt::KeyboardModifiers modifiers, const QString &text, bool isPress);

    // 检查是否具有模拟键盘事件的权限
    bool hasKeyboardControlPermission() const;

signals:
    // 当键盘事件被成功模拟时发出
    void keyEventSimulated(const QString &eventInfo);
    // 当模拟键盘事件失败时发出
    void keyEventFailed(const QString &error);

private:
    // 平台特定的键盘事件模拟实现
    void simulateKeyEvent(int key, Qt::KeyboardModifiers modifiers, const QString &text, bool isPress);

    // 检查权限的私有方法
    bool checkPermissions() const;

    // 将Qt键码转换为平台特定键码
    int qtKeyToNativeKey(int qtKey) const;

    // 当前按下的键状态
    QSet<int> m_pressedKeys;
};

#endif // REMOTEKEYBOARDCONTROLLER_H
