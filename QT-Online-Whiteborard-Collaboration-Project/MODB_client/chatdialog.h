#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();

    void addMessage(const QString &userName, const QString &message, const QString &time = "");
    void setUserName(const QString &name);
    QString getUserName() const;

signals:
    void messageSent(const QString &message);

private slots:
    void onSendClicked();
    void onTextChanged(const QString &text);

private:
    void setupUI();

    QListWidget *m_messageList;
    QLineEdit *m_messageEdit;
    QPushButton *m_sendButton;
    QString m_userName;
};

#endif // CHATDIALOG_H
