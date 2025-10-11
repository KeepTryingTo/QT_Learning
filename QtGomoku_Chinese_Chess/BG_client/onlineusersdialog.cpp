#include "onlineusersdialog.h"
#include <QIcon>
#include <QFile>

OnlineUsersDialog::OnlineUsersDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setWindowTitle("在线用户列表");
    setMinimumSize(300, 400);

    QFile file(":/resources/users_dialog.css");
    if (file.open(QIODevice::ReadOnly)) {
        QString strCss = file.readAll();
        this->setStyleSheet(strCss);
        file.close();
    } else {
        qDebug() << "无法加载CSS文件";
    }
}

OnlineUsersDialog::~OnlineUsersDialog()
{
    // 自动释放子控件
}

void OnlineUsersDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 顶部信息栏
    QHBoxLayout *infoLayout = new QHBoxLayout();
    m_countLabel = new QLabel("在线用户: 0 人", this);
    m_refreshButton = new QPushButton("刷新", this);
    m_refreshButton->setFixedSize(60, 25);

    infoLayout->addWidget(m_countLabel);
    infoLayout->addStretch();
    infoLayout->addWidget(m_refreshButton);

    // 用户列表
    m_usersListWidget = new QListWidget(this);
    m_usersListWidget->setAlternatingRowColors(true);

    mainLayout->addLayout(infoLayout);
    mainLayout->addWidget(m_usersListWidget);

    // 连接信号槽
    connect(m_refreshButton, &QPushButton::clicked, this, &OnlineUsersDialog::onRefreshClicked);
    connect(m_usersListWidget, &QListWidget::itemDoubleClicked, this, &OnlineUsersDialog::onUserItemDoubleClicked);
}

void OnlineUsersDialog::setOnlineUsers(const QStringList &users)
{
    m_onlineUsers = users;
    m_usersListWidget->clear();
    m_usersListWidget->addItems(m_onlineUsers);
    updateUserCount();
}

void OnlineUsersDialog::addUser(const QString &username)
{
    if (!m_onlineUsers.contains(username)) {
        m_onlineUsers.append(username);
        // 重新设置以保持排序
        m_onlineUsers.sort();
        setOnlineUsers(m_onlineUsers);
    }
}

void OnlineUsersDialog::removeUser(const QString &username)
{
    if (m_onlineUsers.contains(username)) {
        m_onlineUsers.removeAll(username);
        setOnlineUsers(m_onlineUsers);
    }
}

void OnlineUsersDialog::clearUsers()
{
    m_onlineUsers.clear();
    m_usersListWidget->clear();
    updateUserCount();
}

QStringList OnlineUsersDialog::getOnlineUsers() const
{
    return m_onlineUsers;
}

int OnlineUsersDialog::getUserCount() const
{
    return m_onlineUsers.size();
}

void OnlineUsersDialog::updateUserCount()
{
    m_countLabel->setText(QString("在线用户: %1 人").arg(m_onlineUsers.size()));
}

void OnlineUsersDialog::onRefreshClicked()
{
    emit refreshRequested();
}

void OnlineUsersDialog::onUserItemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        emit userDoubleClicked(item->text());
    }
}
