#include "onlineusersmanager.h"
#include <QApplication>
#include <QMainWindow>

OnlineUsersManager::OnlineUsersManager(QObject *parent)
    : QObject(parent)
    , m_dialog(nullptr)
{
    // 延迟创建对话框，直到需要时
}

OnlineUsersManager::~OnlineUsersManager()
{
    if (m_dialog) {
        m_dialog->deleteLater();
    }
}

void OnlineUsersManager::showDialog()
{
    if (!m_dialog) {
        // 获取主窗口作为父对象
        QWidget *parentWidget = qobject_cast<QWidget*>(parent());

        if (!parentWidget) {
            // 如果当前对象没有父对象，查找顶层窗口
            QWidgetList widgets = QApplication::topLevelWidgets();
            for (QWidget *widget : widgets) {
                if (widget->inherits("QMainWindow")) {
                    parentWidget = widget;
                    break;
                }
            }
        }

        // 创建对话框
        m_dialog = new OnlineUsersDialog(parentWidget);
        connect(m_dialog, &OnlineUsersDialog::refreshRequested, this, &OnlineUsersManager::onRefreshRequested);
        // ... 其他初始化代码
    }

    m_dialog->show();
    m_dialog->raise();
    m_dialog->activateWindow();
}

// 隐藏窗口
void OnlineUsersManager::hideDialog()
{
    if (m_dialog) {
        m_dialog->hide();
    }
}

bool OnlineUsersManager::isDialogVisible() const
{
    return m_dialog && m_dialog->isVisible();
}

// 设置在线用户列表信息
void OnlineUsersManager::setOnlineUsers(const QStringList &users)
{
    m_onlineUsers = users;
    if (m_dialog) {
        m_dialog->setOnlineUsers(m_onlineUsers);
    }
    emit userCountChanged(m_onlineUsers.size());
}

//添加用户
void OnlineUsersManager::addUser(const QString &username)
{
    if (!m_onlineUsers.contains(username)) {
        m_onlineUsers.append(username);
        m_onlineUsers.sort();
        if (m_dialog) {
            m_dialog->setOnlineUsers(m_onlineUsers);
        }
        emit userCountChanged(m_onlineUsers.size());
    }
}

void OnlineUsersManager::removeUser(const QString &username)
{
    if (m_onlineUsers.contains(username)) {
        m_onlineUsers.removeAll(username);
        if (m_dialog) {
            m_dialog->setOnlineUsers(m_onlineUsers);
        }
        emit userCountChanged(m_onlineUsers.size());
    }
}

void OnlineUsersManager::clearUsers()
{
    m_onlineUsers.clear();
    if (m_dialog) {
        m_dialog->clearUsers();
    }
    emit userCountChanged(0);
}

QStringList OnlineUsersManager::getOnlineUsers() const
{
    return m_onlineUsers;
}

int OnlineUsersManager::getUserCount() const
{
    return m_onlineUsers.size();
}

void OnlineUsersManager::onUserDoubleClicked(const QString &username)
{
    emit userDoubleClicked(username);
}

void OnlineUsersManager::onRefreshRequested()
{
    emit refreshRequested();
}
