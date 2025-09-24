#include "connectdialog.h"
#include <QSettings>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

ConnectDialog::ConnectDialog(QWidget *parent)
    : QDialog(parent)
{
    // 当前弹窗的title，窗口属性，窗口大小
    setWindowTitle("连接到白板服务器");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    setFixedSize(400, 300);

    setupUI();

    // 加载上次的设置，QSettings 是 Qt 中用于持久化存储应用程序设置的类，
    // 它会自动从平台特定的位置（如 Windows 注册表或 macOS 的 plist 文件）读取和写入配置数据。
    QString ip = this -> getLocalIp();
    QSettings settings;
    m_hostEdit->setText(settings.value("Server/host", ip).toString());
    m_portSpinBox->setValue(settings.value("Server/port", 8080).toInt());
    m_roomIdEdit->setText(settings.value("Server/roomId", "").toString());
    // 使用新的随机数生成方式
    QString defaultName = QString("User_%1").arg(QRandomGenerator::global()->bounded(1000));
    m_userNameEdit->setText(settings.value("Server/userName", defaultName).toString());
}

ConnectDialog::~ConnectDialog()
{
    // 保存设置
    QSettings settings;
    settings.setValue("Server/host", m_hostEdit->text());
    settings.setValue("Server/port", m_portSpinBox->value());
    settings.setValue("Server/roomId", m_roomIdEdit->text());
    settings.setValue("Server/userName", m_userNameEdit->text());
    settings.clear();
}

QString ConnectDialog::getLocalIp()
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

// 整个弹窗的界面
void ConnectDialog::setupUI()
{
    // 垂直布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // 设置布局中各个控件之间的间距为 15 像素
    mainLayout->setSpacing(15);
    // 设置布局的内容边距（margin），即布局边界与内部控件之间的空白区域
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 标题
    QLabel *titleLabel = new QLabel("连接到白板服务器", this);
    titleLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");
    mainLayout->addWidget(titleLabel);

    // 表单布局
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    // 服务器地址
    m_hostEdit = new QLineEdit(this);
    m_hostEdit->setPlaceholderText("例如：localhost 或 192.168.1.100");
    formLayout->addRow("服务器地址：", m_hostEdit);

    // 端口号
    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setRange(1, 65535);
    m_portSpinBox->setValue(8080);
    formLayout->addRow("端口号：", m_portSpinBox);

    // 房间ID
    m_roomIdEdit = new QLineEdit(this);
    m_roomIdEdit->setPlaceholderText("请输入已存在的房间号(留空将创建新房间)");
    // 正则表达式，只允许字母数字和连字符
    QRegularExpressionValidator *roomIdValidator = new QRegularExpressionValidator(
        QRegularExpression("[a-zA-Z0-9\\-]*"), this);
    m_roomIdEdit->setValidator(roomIdValidator);
    formLayout->addRow("房间ID：", m_roomIdEdit);

    // 用户名
    m_userNameEdit = new QLineEdit(this);
    m_userNameEdit->setPlaceholderText("输入您的显示名称");
    formLayout->addRow("用户名：", m_userNameEdit);

    mainLayout->addLayout(formLayout);

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    // m_connectButton = new QPushButton("连接", this);
    // m_connectButton->setDefault(true);
    // m_connectButton->setMinimumWidth(100);
    // connect(m_connectButton, &QPushButton::clicked, this, &ConnectDialog::onConnectClicked);

    // m_cancelButton = new QPushButton("取消", this);
    // m_cancelButton->setMinimumWidth(100);
    // connect(m_cancelButton, &QPushButton::clicked, this, &ConnectDialog::onCancelClicked);
    // buttonLayout->addStretch();
    // buttonLayout->addWidget(buttonBox);
    // buttonLayout->addWidget(m_cancelButton);

    // 使用 QDialogButtonBox 替代手动按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConnectDialog::onConnectClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConnectDialog::onCancelClicked);
    // 在布局中添加一个弹性空间（stretch），这个空间会自动扩展以填充可用空间，从而将后续添加到布局中的控件"推"到右侧。
    buttonLayout->addStretch();
    buttonLayout->addWidget(buttonBox);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();
}

void ConnectDialog::onConnectClicked()
{
    std::cout << "onConnectClicked is used" << std::endl;
    std::cout << "IP: " << m_hostEdit->text().trimmed().toStdString() << std::endl;
    std::cout << "user name: " << m_userNameEdit->text().trimmed().toStdString() << std::endl;

    if (m_hostEdit->text().trimmed().isEmpty()) {
        std::cout << "IP is empty, warning" << std::endl;
        QMessageBox::warning(this, "输入错误", "请输入服务器地址");
        m_hostEdit->setFocus();
        return;
    }

    if (m_userNameEdit->text().trimmed().isEmpty()) {
        std::cout << "username is empty, warning" << std::endl;
        QMessageBox::warning(this, "输入错误", "请输入用户名");
        m_userNameEdit->setFocus();
        return;
    }

    std::cout << "input is pass and accept()" << std::endl;
    accept();
    std::cout << "accept() used finished" << std::endl;
}

void ConnectDialog::onCancelClicked()
{
    reject(); // 关闭对话框并返回 Rejected
}

QString ConnectDialog::getServerUrl() const
{
    return QString("ws://%1:%2").arg(m_hostEdit->text()).arg(m_portSpinBox->value());
}

QString ConnectDialog::getHost() const
{
    return m_hostEdit->text();
}

quint16 ConnectDialog::getPort() const
{
    return static_cast<quint16>(m_portSpinBox->value());
}

QString ConnectDialog::getRoomId() const
{
    return m_roomIdEdit->text().trimmed();
}

QString ConnectDialog::getUserName() const
{
    return m_userNameEdit->text().trimmed();
}
