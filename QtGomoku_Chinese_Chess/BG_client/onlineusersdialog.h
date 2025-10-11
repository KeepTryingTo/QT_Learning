#ifndef ONLINEUSERSDIALOG_H
#define ONLINEUSERSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStringList>

class OnlineUsersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OnlineUsersDialog(QWidget *parent = nullptr);
    ~OnlineUsersDialog();

    void setOnlineUsers(const QStringList &users);
    void addUser(const QString &username);
    void removeUser(const QString &username);
    void clearUsers();

    QStringList getOnlineUsers() const;
    int getUserCount() const;

signals:
    void userDoubleClicked(const QString &username);
    void refreshRequested();

private slots:
    void onRefreshClicked();
    void onUserItemDoubleClicked(QListWidgetItem *item);

private:
    void setupUI();
    void updateUserCount();

    QListWidget *m_usersListWidget;
    QLabel *m_countLabel;
    QPushButton *m_refreshButton;
    QStringList m_onlineUsers;
};

#endif // ONLINEUSERSDIALOG_H
