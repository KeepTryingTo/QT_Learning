#include "server.h"
#include "ui_server.h"

Server::Server(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Server)
{
    ui->setupUi(this);

    // 创建指示灯
    m_connectionStatus = new LedIndicator(this);
    // 添加到布局中
    layout()->addWidget(m_connectionStatus);
    // 将指示灯设置为覆盖模式
    m_connectionStatus->setParent(this);
    m_connectionStatus->move(width() - m_connectionStatus->width() - 50,
                             height() - m_connectionStatus->height() - 7);

    this -> setFixedSize(QSize(598, 349));


    // 加载CSS样式
    QFile file(":/resources/windows.css");
    if (file.open(QIODevice::ReadOnly)) {
        QString strCss = file.readAll();
        this->setStyleSheet(strCss);
        file.close();
    } else {
        qDebug() << "无法加载CSS文件";
    }

    m_sendfile = new sendFile();
    this -> is_send = false;
    this -> socket = nullptr;
    this -> getLocalIp();
    m_server = new QWebSocketServer("chat server", QWebSocketServer::NonSecureMode, this);


    // 实时传输画面
    m_screenCapturer = new ScreenCapturer(this);
    connect(m_screenCapturer, &ScreenCapturer::errorOccurred, this, [this](const QString &error) {
        ui->chat_frame->appendPlainText("❌ " + error);
        // 错误消息只需要提醒我一次就可以，所以这里接收到错误消息之后就断开信号和槽的连接
        disconnect(m_screenCapturer, &ScreenCapturer::errorOccurred, this, nullptr); // 断开连接
    });


    // 初始化鼠标控制器
    m_mouseController = new RemoteMouseController(this);
    // 模拟鼠标移动成功输出
    connect(m_mouseController, &RemoteMouseController::mouseEventSimulated,
            this, [this](const QString &info) {
                // ui->chat_frame->appendPlainText("鼠标控制: " + info);
        std::cout<<"鼠标控制: "<<info.toStdString()<<std::endl;
    });
    // 模拟鼠标移动失败输出
    connect(m_mouseController, &RemoteMouseController::mouseEventFailed,
            this, [this](const QString &error) {
                ui->chat_frame->appendPlainText("鼠标控制错误: " + error);
    });

    // 初始化键盘控制器
    m_keyboardController = new RemoteKeyboardController(this);
    connect(m_keyboardController, &RemoteKeyboardController::keyEventSimulated,
            this, [this](const QString &info) {
                // ui->chat_frame->appendPlainText("键盘控制: " + info);
        std::cout<<"键盘控制: "<<info.toStdString()<<std::endl;
            });
    connect(m_keyboardController, &RemoteKeyboardController::keyEventFailed,
            this, [this](const QString &error) {
                ui->chat_frame->appendPlainText("键盘控制错误: " + error);
            });
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

        // 连接新连接信号
        connect(m_server, &QWebSocketServer::newConnection, this, &Server::onNewConnection);

        ui->listen_btn->setEnabled(false);
    } else {
        QMessageBox::warning(this, "❌ 服务器启动失败:", m_server->errorString());
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
        // std::cout<<"接收消息: "<<msg.toStdString()<<std::endl;

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

    // 设置目标socket
    m_screenCapturer->setTargetSocket(socket);

    // 捕获屏幕的类发送一个信号，这个信号包含了捕获的屏幕帧，根据帧信号可以做相应的处理
    connect(m_screenCapturer, &ScreenCapturer::frameCaptured, this, [this](const QPixmap &frame) {
        // 可以在这里添加本地预览逻辑

    });
}

void Server::onClientDisconnected()
{
    m_connectionStatus->setStatus(LedIndicator::BlinkingRed);
    // ui->chat_frame->appendPlainText("❌ 客户端断开连接...");

    if (socket) {
        disconnect(socket, nullptr, this, nullptr); // 断开所有信号连接
        socket->deleteLater();
        socket = nullptr;  // 重要：断开后设为nullptr
    }
}

void Server::onBinaryMessageReceived(const QByteArray &message)
{
    m_connectionStatus->setStatus(LedIndicator::BlinkingGreen);
    // 解析消息类型
    QDataStream stream(message);
    QString messageType;
    stream >> messageType;

    // 接收来自客户端的鼠标移动或者点击事件并进行解析
    if (messageType == "MOUSE_EVENT"){
        processMouseEvent(message);
        return;
    }

    if(messageType == "KEY_EVENT"){
        processKeyEvent(message);
        return;
    }

    if(this -> is_send){
        // 判断客户端是请求还是下载文件（客户端首先请求，然后是下载，但是在客户端我们是合并在一个函数中）
        if (messageType == "FILE_REQUEST") {
            m_sendfile -> processFileRequest(message, socket);
        } else if (messageType == "FILE_CHUNK_ACK") {
            // 处理文件块确认
            if(m_sendfile -> getIsFinished()){
                m_sendfile -> setIsFinished(false);
                this -> is_send = false;
                ui->chat_frame->appendPlainText("✅ 文件发送完成: " + m_sendfile -> getFileName());
                return;
            }
            m_sendfile -> sendFileChunk(socket);
            ui->chat_frame->appendPlainText("📊 发送进度: " + QString::number(m_sendfile -> getProgress()) + "%");
        }

    }else{
        // 接收下载文件
            if (messageType == "FILE_HEADER") {
            processFileHeader(message);
        } else if (messageType == "FILE_CHUNK") {
            processFileChunk(message);
        }
    }
}

// 接收来自客户端的文件
void Server::processFileHeader(const QByteArray &data)
{
    QDataStream stream(data);
    QString messageType;
    // 解析服务端发送的内容，由于服务端首先发送的是文件头部消息，所以这里首先解析头部消息
    stream >> messageType >> receivedFileName >> expectedFileSize;

    if (messageType == "FILE_HEADER") {
        receivedFileSize = 0;
        receivedFileData.clear();

        // ui->chat_frame->appendPlainText("📥 开始接收文件: " + receivedFileName +
        //                                 " (" + QString::number(expectedFileSize) + " bytes)");

        // 确认接收
        QByteArray ack;
        QDataStream ackStream(&ack, QIODevice::WriteOnly);
        // 接收服务端发送的文件头部信息之后，客户端回应响应内容
        ackStream << QString("FILE_ACK") << receivedFileName;

        socket -> sendBinaryMessage(ack);
    }
}

void Server::processFileChunk(const QByteArray &data)
{
    QDataStream stream(data);
    QString messageType;
    QByteArray chunk;
    stream >> messageType >> chunk;

    if (messageType == "FILE_CHUNK") {
        receivedFileData.append(chunk);
        receivedFileSize += chunk.size();

        // 显示进度
        int progress = static_cast<int>((receivedFileSize * 100) / expectedFileSize);
        // ui->chat_frame->appendPlainText("📊 接收进度: " + QString::number(progress) + "%");

        // 发送确认
        QByteArray ack;
        QDataStream ackStream(&ack, QIODevice::WriteOnly);
        // 接收服务端发送的文件头部信息之后，客户端将正式请求下载内容
        ackStream << QString("FILE_CHUNK_ACK");
        socket ->sendBinaryMessage(ack);

        // 检查是否接收完成
        if (receivedFileSize >= expectedFileSize) {
            ui->chat_frame->appendPlainText("✅ 文件接收完成: " + receivedFileName);
            ui->chat_frame->appendPlainText("💾 文件已缓存，点击下载按钮保存到本地");

            // 启用下载按钮
            ui->download_file_btn->setEnabled(true);
        }
    }
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

void Server::on_exit_btn_clicked()
{
    this -> close();
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

// 获取连接质量评估
QString Server::getConnectionQuality()
{
    // 这里可以根据需要实现更复杂的连接质量检测
    if (socket -> state() != QAbstractSocket::ConnectedState) {
        return "无连接";
    }

    // 简单返回"良好"，您可以扩展这个功能
    return "良好 ✅";
}

// 获取连接持续时间
QString Server::getConnectionDuration()
{
    // 如果需要跟踪连接时间，可以在连接成功时记录开始时间
    // 这里简单实现
    if (socket->state() == QAbstractSocket::ConnectedState) {
        return "持续连接中";
    } else {
        return "未连接";
    }
}

void Server::on_search_btn_clicked()
{
    QString info;

    // 客户端连接状态
    info += "🔗 连接状态: " + getSocketStateString(socket->state()) + "\n\n";

    // 服务器连接信息
    if (socket->state() == QAbstractSocket::ConnectedState) {
        info += "🖥️ 服务器地址: " + socket->peerAddress().toString() + "\n";
        info += "🚪 服务器端口: " + QString::number(socket->peerPort()) + "\n";
        info += "📶 连接质量: " + getConnectionQuality() + "\n\n";
    }

    // 连接持续时间（如果需要的话）
    info += "⏱️ 连接时间: " + getConnectionDuration() + "\n";

    QMessageBox::information(this, "聊天信息", info);
}


void Server::on_sendfile_btn_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择要发送的文件",
                                                    QDir::homePath(), "所有文件 (*.*)");

    std::cout<<"开始发送文件 = "<<std::endl;
    if (!filePath.isEmpty()) {
        m_sendfile -> sendfile_(filePath, socket);
        this -> is_send = true;
    }
}

void sleep_ms(int milliseconds) {
    QThread::msleep(milliseconds);
}


void Server::on_download_file_btn_clicked()
{
    // 第一步：点击下载按钮首先向服务端进行请求
    if (socket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "错误", "未连接到服务器，无法请求文件");
        return;
    }

    // 首先睡眠100MS，让第一步请求完成
    sleep_ms(100);

    // 第二步请求完成之后再是开始下载文件
    if (receivedFileData.isEmpty()) {
        QMessageBox::information(this, "提示", "没有可下载的文件");
        return;
    }

    // 保存下载文件到根目录下
    QString savePath = QFileDialog::getSaveFileName(this, "保存文件",
                                                    QDir::homePath() + "/" + receivedFileName);
    if (!savePath.isEmpty()) {
        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(receivedFileData);
            file.close();
            ui->chat_frame->appendPlainText("💾 文件已保存: " + savePath);
        } else {
            QMessageBox::warning(this, "错误", "无法保存文件: " + savePath);
        }
    }
}


void Server::on_start_screen_shared_clicked()
{
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        m_screenCapturer->startCapture();
        ui->chat_frame->appendPlainText("🖥️ 开始屏幕共享");
    }
}


void Server::on_close_shared_btn_clicked()
{
    m_screenCapturer->stopCapture();
    ui->chat_frame->appendPlainText("⏹️ 停止屏幕共享");
}


void Server::processMouseEvent(const QByteArray &data)
{
    QDataStream stream(data);
    QString messageType;
    QPoint position;
    int button;
    QString action;
    // 接收来自客户端的鼠标移动或者点击事件并进行解析
    stream >> messageType >> position >> button >> action;

    if (messageType == "MOUSE_EVENT") {
        m_mouseController->processMouseEvent(position,
                                             static_cast<Qt::MouseButton>(button),
                                             action);
    }
}

// 添加键盘事件处理
void Server::processKeyEvent(const QByteArray &data)
{
    qDebug() << "Received key event data, size:" << data.size();
    QDataStream stream(data);
    QString messageType;
    int key;
    quint32 modifiers;
    QString text;
    bool isPress;

    stream >> messageType >> key >> modifiers >> text >> isPress;

    // 根据客户端传回来的键盘输入，远程服务端处理键盘输入事件
    if (messageType == "KEY_EVENT") {
        m_keyboardController->processKeyEvent(key, static_cast<Qt::KeyboardModifiers>(modifiers), text, isPress);
    }
}


void Server::on_frame_ratio_slider_sliderMoved(int value)
{
    m_screenCapturer->setFrameRate(value);
    ui->frame_ratio_label->setText(QString("%1fps").arg(value));
}


void Server::on_quality_slider_sliderMoved(int value)
{
    m_screenCapturer->setImageQuality(value);
    ui->quality_label->setText(QString("%1%").arg(value));
}

