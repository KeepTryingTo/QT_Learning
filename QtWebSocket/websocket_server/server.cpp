#include "server.h"
#include "ui_server.h"

Server::Server(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Server)
    , currentFile(nullptr)
    , fileSize(0)
    , bytesWritten(0)
{
    ui->setupUi(this);

    // 创建指示灯
    m_connectionStatus = new LedIndicator(this);
    // 添加到布局中
    layout()->addWidget(m_connectionStatus);
    // 将指示灯设置为覆盖模式
    m_connectionStatus->setParent(this);
    m_connectionStatus->move(width() - m_connectionStatus->width() - 30,
                             height() - m_connectionStatus->height() - 5);

    this -> setFixedSize(QSize(510, 374));

    // 加载CSS样式
    QFile file(":/resources/windows.css");
    if (file.open(QIODevice::ReadOnly)) {
        QString strCss = file.readAll();
        this->setStyleSheet(strCss);
        file.close();
    } else {
        qDebug() << "无法加载CSS文件";
    }

    this -> socket = nullptr;
    this -> getLocalIp();
    m_server = new QWebSocketServer("chat server", QWebSocketServer::NonSecureMode, this);

}

Server::~Server()
{
    delete ui;
}

void Server::on_listen_btn_clicked()
{
    const QString ip = ui->ip_text->toPlainText().trimmed();
    const int port = ui->port_box->value();

    // 正在连接服务器
    m_connectionStatus->setStatus(LedIndicator::BlinkingYellow);

    // 验证IP地址
    QHostAddress address;
    if (ip.isEmpty()) {
        address = QHostAddress::Any;
    } else if (!address.setAddress(ip)) {
        QMessageBox::warning(this, "错误", "无效的IP地址格式");
        return;
    }

    // 验证端口
    if (port < 1 || port > 65535) {
        QMessageBox::warning(this, "错误", "端口号必须在1-65535之间");
        return;
    }

    // 启动监听
    if (m_server->listen(address, port)) {
        ui->chat_frame->appendPlainText("✅ 服务器启动成功，正在监听...");
        ui->chat_frame->appendPlainText("📍 地址: " + address.toString());
        ui->chat_frame->appendPlainText("🚪 端口: " + QString::number(port));

        // 连接新连接信号
        connect(m_server, &QWebSocketServer::newConnection, this, &Server::onNewConnection);

        ui->listen_btn->setEnabled(false);
        ui->close_btn->setEnabled(true);
    } else {
        ui->chat_frame->appendPlainText("❌ 服务器启动失败: " + m_server->errorString());
    }
}

void Server::onNewConnection()
{
    m_connectionStatus->setStatus(LedIndicator::BlinkingGreen);

    // 获取新的客户端连接
    QWebSocket *newSocket = m_server->nextPendingConnection();
    if (!newSocket) {
        return;
    }

    // 如果已有连接，先关闭旧连接
    if (socket) {
        // 先断开所有信号连接
        disconnect(socket, &QWebSocket::textMessageReceived, this, nullptr);
        disconnect(socket, &QWebSocket::binaryMessageReceived, this, nullptr);
        disconnect(socket, &QWebSocket::disconnected, this, nullptr);
        disconnect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                   this, nullptr);
        socket->close();
        socket->deleteLater();
        socket = nullptr;
    }

    // 更新socket指针指向新连接
    socket = newSocket;

    ui->chat_frame->appendPlainText("✅ 有新的客户端连接...");
    ui->chat_frame->appendPlainText("📡 客户端地址: " + socket->peerAddress().toString());
    ui->chat_frame->appendPlainText("🚪 客户端端口: " + QString::number(socket->peerPort()));

    // 连接信号槽
    connect(socket, &QWebSocket::textMessageReceived, this, [this](const QString &msg){
        ui->chat_frame->appendPlainText("客户端: " + msg);
    });
    // 添加二进制消息接收连接
    connect(socket, &QWebSocket::binaryMessageReceived, this, &Server::onBinaryMessageReceived);

    connect(socket, &QWebSocket::disconnected, this, &Server::onClientDisconnected);

    // 连接错误信号
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [=](QAbstractSocket::SocketError error){
        if (socket) { // 添加安全检查
            ui->chat_frame->appendPlainText("❌ 连接错误: " + socket->errorString());
        }
    });
}

void Server::onClientDisconnected()
{
    m_connectionStatus->setStatus(LedIndicator::BlinkingRed);
    ui->chat_frame->appendPlainText("❌ 客户端断开连接...");

    // 清理文件传输
    if (currentFile) {
        currentFile->close();
        delete currentFile;
        currentFile = nullptr;
    }
    if (socket) {
        disconnect(socket, nullptr, this, nullptr); // 断开所有信号连接
        socket->deleteLater();
        socket = nullptr;  // 重要：断开后设为nullptr
    }
}

void Server::onBinaryMessageReceived(const QByteArray &message)
{
    // 解析消息类型
    QDataStream stream(message);
    QString messageType;
    stream >> messageType;

    // 判断客户端是请求还是下载文件（客户端首先请求，然后是下载，但是在客户端我们是合并在一个函数中）
    if (messageType == "FILE_REQUEST") {
        processFileRequest(message);
    } else if (messageType == "FILE_CHUNK_ACK") {
        // 处理文件块确认
        sendFileChunk();
    }
}

void Server::processFileRequest(const QByteArray &data)
{
    QDataStream stream(data);
    QString messageType;
    QString fileName;
    qint64 fileSize;

    stream >> messageType >> fileName >> fileSize;

    // 请求文件的时候服务端输出信息
    if (messageType == "FILE_REQUEST") {
        ui->chat_frame->appendPlainText("📁 客户端请求文件: " + fileName +
                                        " (" + QString::number(fileSize) + " bytes)");

        // 这里可以添加文件存在性检查、权限验证等
        sendFile(fileName);
    }
}

void Server::sendFile(const QString &filePath)
{
    // 首先判断连接状态
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "错误", "没有客户端连接，无法发送文件");
        return;
    }

    // 获取文件信息
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "错误", "文件不存在: " + filePath);
        return;
    }

    // 清理之前的文件传输
    if (currentFile) {
        currentFile->close();
        delete currentFile;
    }

    // 创建文件对象，打开文件判断
    currentFile = new QFile(filePath);
    if (!currentFile->open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开文件: " + filePath);
        delete currentFile;
        currentFile = nullptr;
        return;
    }
    // D:\\SoftwareFamily\\QT\\projects\\QtWebSocket\\websocket_server\\github-recovery-codes.txt

    // 当前文件大小以及文件名称
    fileSize = currentFile->size();
    bytesWritten = 0;
    currentFileName = fileInfo.fileName();

    // 发送文件头信息，仅仅是写入流中
    QByteArray header;
    QDataStream stream(&header, QIODevice::WriteOnly);
    // 将关键词“FILE_HEADER”添加到流中
    stream << QString("FILE_HEADER") << currentFileName << fileSize;

    // 发送文件头部信息二进制字节流
    socket->sendBinaryMessage(header);
    ui->chat_frame->appendPlainText("📤 开始发送文件: " + currentFileName +
                                    " (" + QString::number(fileSize) + " bytes)");

    // 其次是发送具体文件信息，开始发送第一个数据块（分批次发送文件）
    sendFileChunk();
}

void Server::sendFileChunk()
{
    if (!currentFile || !socket) return;

    const qint64 chunkSize = 64 * 1024; // 64KB 块大小
    QByteArray chunk = currentFile->read(chunkSize);

    if (chunk.isEmpty()) {
        // 文件发送完成
        currentFile->close();
        delete currentFile;
        currentFile = nullptr;

        ui->chat_frame->appendPlainText("✅ 文件发送完成: " + currentFileName);
        return;
    }

    // 发送数据块
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream << QString("FILE_CHUNK") << chunk;

    socket->sendBinaryMessage(message);
    bytesWritten += chunk.size();

    // 显示进度
    int progress = static_cast<int>((bytesWritten * 100) / fileSize);
    ui->chat_frame->appendPlainText("📊 发送进度: " + QString::number(progress) + "%");
}

QString Server::getLocalIp()
{
    QString hostName = QHostInfo::localHostName();

    ui->chat_frame->appendPlainText("");
    ui->chat_frame->appendPlainText("🖥️ 本机网络详细信息");
    ui->chat_frame->appendPlainText("📛 主机名称: " + hostName);
    ui->chat_frame->appendPlainText("");

    QStringList availableIps;
    QString preferredIp;

    // 获取所有网络接口
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach (const QNetworkInterface &interface, interfaces) {
        // 跳过回环和未启用的接口
        if (interface.flags().testFlag(QNetworkInterface::IsLoopBack) ||
            !interface.flags().testFlag(QNetworkInterface::IsUp) ||
            !interface.flags().testFlag(QNetworkInterface::IsRunning)) {
            continue;
        }

        ui->chat_frame->appendPlainText("📡 网络接口: " + interface.humanReadableName());

        // 获取该接口的所有IP地址
        QList<QNetworkAddressEntry> entries = interface.addressEntries();
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
        ui->ip_text->setPlainText("127.0.0.1");
        return "127.0.0.1";
    }

    ui->chat_frame->appendPlainText("✅ 推荐服务器地址: " + preferredIp);
    ui->ip_text->setPlainText(preferredIp);

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


void Server::on_close_btn_clicked()
{
    if (m_server->isListening()) {
        m_server->close();
        ui->chat_frame->appendPlainText("🛑 服务器已停止监听");

        // 断开所有客户端连接
        if (socket) {
            disconnect(socket, nullptr, this, nullptr);
            socket->close();
            socket->deleteLater();
            socket = nullptr;
        }

        // 更新指示灯状态
        m_connectionStatus->setStatus(LedIndicator::Red);

        ui->listen_btn->setEnabled(true);
        ui->close_btn->setEnabled(false);
    }
}


void Server::on_search_btn_clicked()
{
    QString info;

    // 服务器状态
    info += "🖥️ 服务器状态: ";
    info += m_server->isListening() ? "✅ 正在监听" : "❌ 已停止";
    info += "\n";

    // 监听地址和端口
    if (m_server->isListening()) {
        info += "📍 监听地址: " + m_server->serverAddress().toString() + "\n";
        info += "🚪 监听端口: " + QString::number(m_server->serverPort()) + "\n";
    }

    // 客户端连接信息
    info += "👥 客户端连接: ";
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        info += "✅ 已连接 (" + QString::number(1) + "个)\n";
        info += "   📍 客户端IP: " + socket->peerAddress().toString() + "\n";
        info += "   🚪 客户端端口: " + QString::number(socket->peerPort()) + "\n";
        info += "   🔗 连接状态: " + getSocketStateString(socket->state()) + "\n";
    } else {
        info += "❌ 无连接\n";
    }

    // 消息统计（如果需要的话）
    info += "💬 总消息数: " + QString::number(ui->chat_frame->toPlainText().count("\n") + 1) + "\n";

    QMessageBox::information(this, "聊天信息", info);
}


void Server::on_clear_btn_clicked()
{
    ui->chat_frame->clear();
}


void Server::on_send_btn_clicked()
{
    // 改进的连接检查
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "错误", "没有客户端连接，无法发送消息");

        // 更新指示灯状态
        if (m_server->isListening()) {
            m_connectionStatus->setStatus(LedIndicator::BlinkingYellow);
        } else {
            m_connectionStatus->setStatus(LedIndicator::Red);
        }

        return;
    }

    QString msg = ui->input_text->toPlainText().trimmed();
    if (msg.isEmpty()) {
        return;
    }

    ui->chat_frame->appendPlainText("服务端: " + msg);
    socket->sendTextMessage(msg);
    ui->input_text->clear();
}


void Server::on_exit_btn_clicked()
{
    // 先关闭服务器再退出
    if (m_server->isListening()) {
        m_server->close();
    }
    if (socket) {
        socket->close();
    }
    this->close();
}


void Server::on_send_file_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择要发送的文件",
                                                    QDir::homePath(), "所有文件 (*.*)");
    if (!filePath.isEmpty()) {
        sendFile(filePath);
    }
}

