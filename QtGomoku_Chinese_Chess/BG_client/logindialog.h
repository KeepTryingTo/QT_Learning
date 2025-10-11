// LoginDialog.h
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QPropertyAnimation>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlField>
#include <QFile>
#include <QIcon>
#include <QCoreApplication>

#include <map>

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    QString getCurrentUsername() const { return m_editUsername->text().trimmed(); }

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onTextChanged();
    void onExitClicked();  // 退出按钮的槽函数

signals:
    void emitClose();

private:
    QLineEdit *m_editUsername;
    QLineEdit *m_editPassword;
    QLabel *m_labelTitle;
    QCheckBox *m_checkRemember;

    QSqlDatabase m_db;

    void initDatabase();
    void loadUserData();
    void applyModernStyle();
    QString hashPassword(const QString &password);
    bool userExists(const QString &username);
    void onForgotPassword();
    void onTogglePassword();

    // 保存用户名和密码
    std::map<QString, QString>user_info;
};
