#include "chatdialog.h"
#include <QDateTime>
#include <QScrollBar>

ChatDialog::ChatDialog(QWidget *parent)
    : QDialog(parent)
    , m_userName("User")
{
    setWindowTitle("聊天室");
    setMinimumSize(400, 300);
    setMaximumSize(600, 500);

    setupUI();
}

ChatDialog::~ChatDialog()
{
}

void ChatDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 消息显示区域
    m_messageList = new QListWidget(this);
    m_messageList->setStyleSheet(
        "QListWidget {"
        "    background-color: #f8f9fa;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 5px;"
        "}"
        "QListWidget::item {"
        "    border-bottom: 1px solid #e9ecef;"
        "    padding: 8px;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #e3f2fd;"
        "}"
        );
    mainLayout->addWidget(m_messageList);

    // 输入区域的水平布局
    QHBoxLayout *inputLayout = new QHBoxLayout();

    // 输入聊天信息框
    m_messageEdit = new QLineEdit(this);
    m_messageEdit->setPlaceholderText("输入消息...");
    m_messageEdit->setStyleSheet(
        "QLineEdit {"
        "    border: 1px solid #ced4da;"
        "    border-radius: 3px;"
        "    padding: 6px;"
        "}"
        );

    // 按下enter键可以发送消息，也可以按下发送“按钮”可以发送消息
    connect(m_messageEdit, &QLineEdit::textChanged, this, &ChatDialog::onTextChanged);
    connect(m_messageEdit, &QLineEdit::returnPressed, this, &ChatDialog::onSendClicked);

    m_sendButton = new QPushButton("发送", this);
    m_sendButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #007bff;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 3px;"
        "    padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #0056b3;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #6c757d;"
        "}"
        );
    m_sendButton->setEnabled(false);
    connect(m_sendButton, &QPushButton::clicked, this, &ChatDialog::onSendClicked);

    inputLayout->addWidget(m_messageEdit, 1);
    inputLayout->addWidget(m_sendButton);

    mainLayout->addLayout(inputLayout);

    setLayout(mainLayout);
}

void ChatDialog::addMessage(const QString &userName, const QString &message, const QString &time)
{
    QString displayTime = time.isEmpty() ? QDateTime::currentDateTime().toString("HH:mm:ss") : time;

    QListWidgetItem *item = new QListWidgetItem();
    QString formattedMessage = QString("[%1] %2: %3")
                                   .arg(displayTime)
                                   .arg(userName)
                                   .arg(message);

    item->setText(formattedMessage);

    // 区分自己和他人的消息
    if (userName == m_userName) {
        item->setTextAlignment(Qt::AlignRight);
        item->setBackground(QColor(225, 245, 254)); // 浅蓝色背景
    } else {
        item->setTextAlignment(Qt::AlignLeft);
        item->setBackground(QColor(255, 255, 255)); // 白色背景
    }
    // 添加到聊天框列表中
    m_messageList->addItem(item);

    // 自动滚动到底部
    m_messageList->scrollToBottom();
}

void ChatDialog::setUserName(const QString &name)
{
    m_userName = name;
}

QString ChatDialog::getUserName() const
{
    return m_userName;
}

// 点击发送按钮执行的槽函数
void ChatDialog::onSendClicked()
{
    QString message = m_messageEdit->text().trimmed();
    if (!message.isEmpty()) {
        // 通过信号将要发送的消息发送给客户端，客户端通过m_websocketManager将消息发送出去
        emit messageSent(message);
        // 清除聊天输入框
        m_messageEdit->clear();
    }
}

void ChatDialog::onTextChanged(const QString &text)
{
    // 如果聊天输入没有要发送的消息，就阻止发送按钮发送消息
    m_sendButton->setEnabled(!text.trimmed().isEmpty());
}
