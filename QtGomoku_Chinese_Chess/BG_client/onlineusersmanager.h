#ifndef ONLINEUSERSMANAGER_H
#define ONLINEUSERSMANAGER_H

#include <QObject>
#include <QStringList>
#include "onlineusersdialog.h"

class OnlineUsersManager : public QObject
{
    Q_OBJECT

public:
    explicit OnlineUsersManager(QObject *parent = nullptr);
    ~OnlineUsersManager();

    void showDialog();
    void hideDialog();
    bool isDialogVisible() const;

    void setOnlineUsers(const QStringList &users);
    void addUser(const QString &username);
    void removeUser(const QString &username);
    void clearUsers();

    QStringList getOnlineUsers() const;
    int getUserCount() const;

signals:
    void userDoubleClicked(const QString &username);
    void refreshRequested();
    void userCountChanged(int count);

private slots:
    void onUserDoubleClicked(const QString &username);
    void onRefreshRequested();

private:
    OnlineUsersDialog *m_dialog;
    QStringList m_onlineUsers;
};

#endif // ONLINEUSERSMANAGER_H
