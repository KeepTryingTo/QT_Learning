#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QHostInfo>
#include <QNetworkInterface>
#include <qrandom.h>
#include <iostream>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QRandomGenerator>

class ConnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectDialog(QWidget *parent = nullptr);
    ~ConnectDialog();

    QString getLocalIp();

    // 获得IP:Port
    QString getServerUrl() const;
    QString getHost() const;
    quint16 getPort() const;
    // 获得房间号以及用户名
    QString getRoomId() const;
    QString getUserName() const;

    void setIP(const QString & ip){
        this -> m_hostEdit->setText(ip);
    }
    void setPort(const qint64 & port){
        this -> m_portSpinBox->setValue(port);
    }

private slots:
    void onConnectClicked();
    void onCancelClicked();

private:
    QLineEdit *m_hostEdit;
    QSpinBox *m_portSpinBox;
    QLineEdit *m_roomIdEdit;
    QLineEdit *m_userNameEdit;
    QPushButton *m_connectButton;
    QPushButton *m_cancelButton;

    void setupUI();
};

#endif // CONNECTDIALOG_H
