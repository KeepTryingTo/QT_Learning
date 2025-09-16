#include "remotekeyboardcontroller.h"
#include <QDebug>
#include <QSet>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <X11/Xlib.h>
#include <X11/keysym.h>
#elif defined(Q_OS_MAC)
#include <Carbon/Carbon.h>
#endif

RemoteKeyboardController::RemoteKeyboardController(QObject *parent) : QObject(parent)
{
}

void RemoteKeyboardController::processKeyEvent(int key, Qt::KeyboardModifiers modifiers, const QString &text, bool isPress)
{
    if (!hasKeyboardControlPermission()) {
        emit keyEventFailed("没有足够的权限控制键盘");
        return;
    }

    try {
        // 模拟键盘输入
        simulateKeyEvent(key, modifiers, text, isPress);

        // 判断是按下键盘还是释放键盘
        QString action = isPress ? "按下" : "释放";
        QString keyInfo = QString("键: %1, 修饰键: %2, 文本: '%3'")
                              .arg(key)
                              .arg(modifiers)
                              .arg(text);

        emit keyEventSimulated(QString("键盘事件: %1 - %2").arg(action).arg(keyInfo));

        // 更新按键状态
        if (isPress) {
            m_pressedKeys.insert(key);
        } else {
            m_pressedKeys.remove(key);
        }
    } catch (...) {
        emit keyEventFailed("模拟键盘事件时发生异常");
    }
}

bool RemoteKeyboardController::hasKeyboardControlPermission() const
{
    return checkPermissions();
}

void RemoteKeyboardController::simulateKeyEvent(int key, Qt::KeyboardModifiers modifiers, const QString &text, bool isPress)
{
#ifdef Q_OS_WINDOWS
    // Windows实现
    INPUT input;
    input.type = INPUT_KEYBOARD; // 键盘输入事件
    input.ki.wVk = qtKeyToNativeKey(key); // 转码操作
    input.ki.wScan = 0; // 硬件扫描
    input.ki.dwFlags = isPress ? 0 : KEYEVENTF_KEYUP; // 按下还是弹起
    input.ki.time = 0; // 时间戳，系统自动填充
    input.ki.dwExtraInfo = 0; // 附加信息

    // 处理修饰键
    if (modifiers & Qt::ShiftModifier) {
        input.ki.wVk = VK_SHIFT;          // 设置Shift键的虚拟键码
        SendInput(1, &input, sizeof(INPUT)); // 发送Shift按下事件
    }
    if (modifiers & Qt::ControlModifier) {
        input.ki.wVk = VK_CONTROL;        // 设置Ctrl键的虚拟键码
        SendInput(1, &input, sizeof(INPUT)); // 发送Ctrl按下事件
    }
    if (modifiers & Qt::AltModifier) {
        input.ki.wVk = VK_MENU;           // 设置Alt键的虚拟键码
        SendInput(1, &input, sizeof(INPUT)); // 发送Alt按下事件
    }

    // 发送主键
    /*
        ​​系统输入队列​​：事件会被插入Windows系统的原始输入流（RAW Input Stream）
        ​​前台窗口​​: 最终由当前获得焦点的应用程序接收（与发送进程无关）
        ​​所有监听输入的进程​​：包括系统级钩子程序（如屏幕键盘、远程控制软件）
    */
    input.ki.wVk = qtKeyToNativeKey(key);
    SendInput(1, &input, sizeof(INPUT));

#elif defined(Q_OS_LINUX)
    // Linux实现 (需要X11)
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        emit keyEventFailed("无法打开X11显示");
        return;
    }

    KeySym keysym = XStringToKeysym(QString(key).toLatin1().data());
    KeyCode keycode = XKeysymToKeycode(display, keysym);

    if (keycode == 0) {
        emit keyEventFailed("无法转换键码");
        XCloseDisplay(display);
        return;
    }

    XTestFakeKeyEvent(display, keycode, isPress ? True : False, CurrentTime);
    XFlush(display);
    XCloseDisplay(display);

#elif defined(Q_OS_MAC)
    // macOS实现
    CGEventRef event;
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStatePrivate);

    if (isPress) {
        event = CGEventCreateKeyboardEvent(source, qtKeyToNativeKey(key), true);
    } else {
        event = CGEventCreateKeyboardEvent(source, qtKeyToNativeKey(key), false);
    }

    // 设置修饰键
    CGEventSetFlags(event, modifiers);
    CGEventPost(kCGSessionEventTap, event);
    CFRelease(event);
    CFRelease(source);
#endif
}

bool RemoteKeyboardController::checkPermissions() const
{
#ifdef Q_OS_WINDOWS
    // Windows下通常需要管理员权限
    return true; // 简化实现，实际应用中需要检查权限
#else
    return true;
#endif
}

int RemoteKeyboardController::qtKeyToNativeKey(int qtKey) const
{
#ifdef Q_OS_WINDOWS
    // Windows键码转换
    switch (qtKey) {
    case Qt::Key_Shift: return VK_SHIFT;
    case Qt::Key_Control: return VK_CONTROL;
    case Qt::Key_Alt: return VK_MENU;
    case Qt::Key_Space: return VK_SPACE;
    case Qt::Key_Enter: return VK_RETURN;
    case Qt::Key_Backspace: return VK_BACK;
    case Qt::Key_Tab: return VK_TAB;
    case Qt::Key_Escape: return VK_ESCAPE;
    // 添加更多键码转换...
    default: return qtKey; // 对于字母数字键，Qt键码与Windows相同
    }
#elif defined(Q_OS_LINUX)
    // Linux键码转换
    return qtKey; // 简化实现，实际需要更复杂的转换
#elif defined(Q_OS_MAC)
    // macOS键码转换
    return qtKey; // 简化实现，实际需要更复杂的转换
#endif
}
