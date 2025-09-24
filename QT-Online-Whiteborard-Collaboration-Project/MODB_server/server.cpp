#include "server.h"
#include "ui_server.h"

Server::Server(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Server)
    , m_webSocketServer(new WebSocketServer(this))
    , m_isServerRunning(false)
{
    ui->setupUi(this);

    // 创建指示灯
    m_connectionStatus = new LedIndicator(this);
    // 添加到布局中
    layout()->addWidget(m_connectionStatus);

    // 将指示灯设置为覆盖模式
    m_connectionStatus->setParent(this);
    m_connectionStatus->move(width() - m_connectionStatus->width() - 10,
                             height() - m_connectionStatus->height() - 5);

    this -> setFixedSize(QSize(522, 342));

    // 加载CSS样式
    QFile file(":/resources/windows.css");
    if (file.open(QIODevice::ReadOnly)) {
        QString strCss = file.readAll();
        this->setStyleSheet(strCss);
        file.close();
    } else {
        qDebug() << "无法加载CSS文件";
    }

    // 设置初始IP地址
    ui->ip_text->setText(getLocalIp());
    ui->port_box->setValue(8080);

    // 连接WebSocketServer信号
    setupConnections();

    // 加载上次的设置
    QSettings settings;
    ui->ip_text->setText(settings.value("Server/ip", getLocalIp()).toString());
    ui->port_box->setValue(settings.value("Server/port", "8080").toInt());

    // 初始化状态
    updateServerStatus(false);


    // 定时的更新当前和服务端连接客户端数量
    m_timer = new QTimer(this);
    m_timer ->setInterval(5000);// 5秒自动检查一次
    connect(m_timer, &QTimer::timeout, this, &Server::diplayClientCounts);
    m_timer -> start();// 启动定时器
}

Server::~Server()
{
    // 保存设置
    QSettings settings;
    settings.setValue("Server/ip", ui->ip_text->toPlainText().trimmed());
    settings.setValue("Server/port", ui->port_box->text());

    if (m_isServerRunning) {
        m_webSocketServer->stopServer();
    }
    delete ui;
}

void Server::diplayClientCounts(){
    // 更新连接数显示
    int clientCount = m_webSocketServer->getClientCount();
    ui->status_label->setText(QString("服务器状态: 运行中 (%1 个客户端连接)").arg(clientCount));
}

QString Server::getLocalIp()
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
                ui->chat_frame->appendPlainText("   " + displayText);
            }
        }
        ui->chat_frame->appendPlainText("");
    }

    if (availableIps.isEmpty()) {
        ui->chat_frame->appendPlainText("❌ 未找到可用IPv4地址");
        ui->chat_frame->appendPlainText("💡 使用回环地址: 127.0.0.1");
        return "127.0.0.1";
    }

    return preferredIp;
}

// 辅助函数：获取连接状态字符串
QString Server::getSocketStateString(QAbstractSocket::SocketState state)
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


void Server::setupConnections()
{
    // 连接WebSocketServer信号
    connect(m_webSocketServer, &WebSocketServer::serverStarted, this, &Server::onServerStarted);
    connect(m_webSocketServer, &WebSocketServer::serverStopped, this, &Server::onServerStopped);
    connect(m_webSocketServer, &WebSocketServer::clientConnected, this, &Server::onClientConnected);
    connect(m_webSocketServer, &WebSocketServer::clientDisconnected, this, &Server::onClientDisconnected);
    connect(m_webSocketServer, &WebSocketServer::errorOccurred, this, &Server::onErrorOccurred);
    connect(m_webSocketServer, &WebSocketServer::messageReceived, this, &Server::onMessageReceived);
}

void Server::on_listen_btn_clicked()
{
    QString ip = ui->ip_text->toPlainText().trimmed();
    QString portText = ui->port_box->text().trimmed();

    if (ip.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入有效的IP地址");
        ui->ip_text->setFocus();
        return;
    }

    if (portText.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入端口号");
        ui->port_box->setFocus();
        return;
    }

    quint16 port = portText.toUShort();
    if (port == 0) {
        QMessageBox::warning(this, "输入错误", "请输入有效的端口号 (1-65535)");
        ui->port_box->setFocus();
        return;
    }

    // 启动WebSocket服务器
    if (m_webSocketServer->startServer(ip, port)) {
        appendLog(QString("[%1] 服务器启动成功，监听端口: %2")
                      .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                      .arg(port));
    } else {
        appendLog(QString("[%1] ❌ 服务器启动失败")
                      .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")));
    }
}

void Server::on_close_btn_clicked()
{
    if (m_isServerRunning) {
        m_webSocketServer->stopServer();
        appendLog(QString("[%1] 服务器已停止")
                      .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")));
    }
}

void Server::on_search_btn_clicked()
{
    // 搜索并显示本机IP地址
    QString localIp = getLocalIp();
    ui->ip_text->setText(localIp);
    appendLog(QString("[%1] 检测到本机IP: %2")
                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                  .arg(localIp));

}

void Server::on_exit_btn_clicked()
{
    if (m_isServerRunning) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "确认退出",
            "服务器正在运行，确定要退出吗？",
            QMessageBox::Yes | QMessageBox::No
            );

        if (reply == QMessageBox::No) {
            return;
        }
        m_webSocketServer->stopServer();
    }
    // 退出当前主程序
    qApp->quit();
}

void Server::onServerStarted()
{
    m_connectionStatus->setStatus(LedIndicator::BlinkingYellow);
    m_isServerRunning = true;
    updateServerStatus(true);
    appendLog(QString("[%1] 服务器开始监听")
                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")));
}

void Server::onServerStopped()
{
    m_connectionStatus->setStatus(LedIndicator::BlinkingRed);
    m_isServerRunning = false;
    updateServerStatus(false);
    appendLog(QString("[%1] 服务器已停止")
                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")));
}

void Server::onClientConnected(const QString &clientId)
{
    m_connectionStatus->setStatus(LedIndicator::BlinkingGreen);
    ui->chat_frame->appendPlainText("✅ 有新的客户端连接...");
    appendLog(QString("[%1] 客户端连接: %2")
                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                  .arg(clientId));

}

void Server::onClientDisconnected(const QString &clientId)
{
    if(m_webSocketServer->getClientCount() == 0){
        m_connectionStatus->setStatus(LedIndicator::BlinkingRed);
    }
    appendLog(QString("[%1] 客户端断开: %2")
                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                  .arg(clientId));

}

void Server::onErrorOccurred(const QString &errorMessage)
{
    appendLog(QString("[%1] 错误: %2")
                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                  .arg(errorMessage));
}

// 处理来自客户端的数据
void Server::onMessageReceived(const NetworkMessage &message)
{
    // 处理接收到的消息,比如消息类型为6，表示绘图操作；消息类型为12，表示连接检测（Heartbeat）
    appendLog(QString("[%1] 收到消息: 类型=%2, 发送者=%3")
                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                  .arg(message.type)
                  .arg(message.senderId));
}

void Server::updateServerStatus(bool isRunning)
{
    m_isServerRunning = isRunning;

    // 更新按钮状态
    ui->listen_btn->setEnabled(!isRunning);
    ui->close_btn->setEnabled(isRunning);
    ui->ip_text->setEnabled(!isRunning);
    ui->port_box->setEnabled(!isRunning);

    // 更新状态标签
    if (isRunning) {
        int clientCount = m_webSocketServer->getClientCount();
        ui->status_label->setText(QString("服务器状态: 运行中 (%1 个客户端连接)").arg(clientCount));
        ui->status_label->setStyleSheet("QLabel { color: green; font-weight: bold; }");
    } else {
        ui->status_label->setText("服务器状态: 未启动");
        ui->status_label->setStyleSheet("QLabel { color: gray; font-weight: bold; }");
    }
}

void Server::appendLog(const QString &message)
{
    // 添加到日志显示区域（假设您有一个QTextEdit或QPlainTextEdit用于显示日志）
    // 如果您的UI中没有日志显示控件，可以添加一个或者使用其他方式显示
    if (ui->chat_frame) {
        ui->chat_frame->appendPlainText(message);
    } else {
        qDebug() << message; // 如果没有UI控件，输出到控制台
    }
}


