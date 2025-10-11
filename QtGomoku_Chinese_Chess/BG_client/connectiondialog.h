#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QIntValidator>
#include <QtWebSockets/QtWebSockets>

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget *parent = nullptr);

    QString getIpAddress() const;
    quint16 getPort() const;

    static QUrl getConnectionInfo(QWidget *parent = nullptr);

    QString getLocalIp();
    QString getSocketStateString(QAbstractSocket::SocketState state);

private:
    QLineEdit *m_ipEdit;
    QLineEdit *m_portEdit;
};

#endif // CONNECTIONDIALOG_H
