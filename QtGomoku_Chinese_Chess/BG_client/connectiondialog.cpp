#include "connectiondialog.h"


ConnectionDialog::ConnectionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("连接服务器");
    setModal(true);
    setFixedSize(300, 150);

    // 创建控件
    m_ipEdit = new QLineEdit(this);
    m_ipEdit->setPlaceholderText("例如: 127.0.0.1");

    QString ip = getLocalIp();
    m_ipEdit->setText(ip);

    // IP地址验证器
    QRegularExpression ipRegex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipRegex, this);
    m_ipEdit->setValidator(ipValidator);

    m_portEdit = new QLineEdit(this);
    int port = 8080;
    m_portEdit->setPlaceholderText("例如: 8080");
    m_portEdit->setText(QString("%1").arg(port));
    m_portEdit->setValidator(new QIntValidator(1, 65535, this));

    // 创建布局
    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow("服务器IP:", m_ipEdit);
    formLayout->addRow("端口号:", m_portEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

QString ConnectionDialog::getIpAddress() const
{
    return m_ipEdit->text().trimmed();
}

quint16 ConnectionDialog::getPort() const
{
    return m_portEdit->text().toUShort();
}

QUrl ConnectionDialog::getConnectionInfo(QWidget *parent)
{
    ConnectionDialog dialog(parent);
    if (dialog.exec() == QDialog::Accepted) {
        QString ip = dialog.getIpAddress();
        quint16 port = dialog.getPort();

        if (!ip.isEmpty() && port > 0) {
            return QUrl(QString("ws://%1:%2").arg(ip).arg(port));
        }
    }
    return QUrl();
}

QString ConnectionDialog::getLocalIp()
{
    QString hostName = QHostInfo::localHostName();

    QStringList availableIps;
    QString preferredIp;

    // 获取所有网络接口
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach (const QNetworkInterface &interfac, interfaces) {
        // 跳过回环和未启用的接口
        if (interfac.flags().testFlag(QNetworkInterface::IsLoopBack) ||
            !interfac.flags().testFlag(QNetworkInterface::IsUp) ||
            !interfac.flags().testFlag(QNetworkInterface::IsRunning)) {
            continue;
        }

        // ui->chat_frame->appendPlainText("📡 网络接口: " + interface.humanReadableName());

        // 获取该接口的所有IP地址
        QList<QNetworkAddressEntry> entries = interfac.addressEntries();
        foreach (const QNetworkAddressEntry &entry, entries) {
            QHostAddress ip = entry.ip();
            // 判断当前IP协议是否为IPv4
            if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                QString ipStr = ip.toString();
                availableIps.append(ipStr);

                QString displayText;
                if (ip.isLoopback()) {
                    displayText = "➰ 回环: " + ipStr;
                } else if (ip.isInSubnet(QHostAddress("192.168.0.0"), 16)) {
                    displayText = "🏠 局域网: " + ipStr;
                    if (preferredIp.isEmpty()) preferredIp = ipStr;
                } else if (ip.isInSubnet(QHostAddress("10.0.0.0"), 8)) {
                    displayText = "🏠 局域网: " + ipStr;
                    if (preferredIp.isEmpty()) preferredIp = ipStr;
                } else if (ip.isInSubnet(QHostAddress("172.16.0.0"), 12)) {
                    displayText = "🏠 局域网: " + ipStr;
                    if (preferredIp.isEmpty()) preferredIp = ipStr;
                } else if (ip.isInSubnet(QHostAddress("169.254.0.0"), 16)) {
                    displayText = "🔗 链路本地: " + ipStr;
                } else {
                    displayText = "🌍 公网: " + ipStr;
                    if (preferredIp.isEmpty()) preferredIp = ipStr;
                }

                // 显示子网掩码
                displayText += " / " + entry.netmask().toString();
                // ui->chat_frame->appendPlainText("   " + displayText);
            }
        }
        // ui->chat_frame->appendPlainText("");
    }

    if (availableIps.isEmpty()) {
        return "127.0.0.1";
    }

    return preferredIp;
}

// 辅助函数：获取连接状态字符串
QString ConnectionDialog::getSocketStateString(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState: return "❌ 未连接";
    case QAbstractSocket::HostLookupState: return "🔍 正在查找主机...";
    case QAbstractSocket::ConnectingState: return "🔄 正在连接...";
    case QAbstractSocket::ConnectedState: return "✅ 已连接";
    case QAbstractSocket::ClosingState: return "⏹️ 正在关闭...";
    default: return "❓ 未知状态";
    }
}
