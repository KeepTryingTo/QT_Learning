#include "client.h"
#include "ui_client.h"

Client::Client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Client)
{
    ui->setupUi(this);

    // 创建指示灯
    m_connectionStatus = new LedIndicator(this);
    // 添加到布局中
    layout()->addWidget(m_connectionStatus);

    // 将指示灯设置为覆盖模式
    m_connectionStatus->setParent(this);
    m_connectionStatus->move(width() - m_connectionStatus->width() - 200,
                             height() - m_connectionStatus->height() - 10);

    this -> setFixedSize(QSize(596, 349));

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

    m_client = new QWebSocket();

    this -> getLocalIp();
    // 重要：禁用代理，直接连接
    m_client->setProxy(QNetworkProxy::NoProxy);

    // 重要：禁用代理，直接连接
    m_client->setProxy(QNetworkProxy::NoProxy);
    // 连接信号槽
    connect(m_client, &QWebSocket::connected, this, &Client::onConnected);
    connect(m_client, &QWebSocket::disconnected, this, &Client::onDisconnected);
    connect(m_client, &QWebSocket::textMessageReceived, this, &Client::onTextMessageReceived);
    connect(m_client, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &Client::onError);

    // 连接二进制消息信号
    connect(m_client, &QWebSocket::binaryMessageReceived, this, &Client::onBinaryMessageReceived);

    // 实例化一个布局窗口
    m_remoteScreen = new RemoteScreenWidget();
    // 设置远程屏幕窗口属性
    m_remoteScreen->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    m_remoteScreen->resize(800, 600);
    // 将远程屏幕布局设置为覆盖模式
    // m_remoteScreen->show();

    // 创建并启动渲染线程
    m_renderThread = new RenderThread(m_remoteScreen);
    // 当信号跨线程发射时，Qt会自动使用Qt::QueuedConnection方式，将信号转换为事件放入主线程的事件队列
    connect(m_renderThread, &RenderThread::renderRequest,
            m_remoteScreen, QOverload<>::of(&RemoteScreenWidget::update)); // 解决重载函数歧义的 Qt 语法

    // 调用setVisible函数之后，会发送响应的信号给onScreenVisibilityChanged，从而达到显示远程屏幕还是屏蔽
    connect(m_remoteScreen, &RemoteScreenWidget::visibilityChanged,
            this, &Client::onScreenVisibilityChanged);

    // 启动线程之后就不断地发送信号RenderThread::renderRequest
    m_renderThread->start();

    // 连接远程屏幕的鼠标事件
    connect(m_remoteScreen, &RemoteScreenWidget::remoteMouseEvent,
            this, &Client::onRemoteMouseEvent);

    // 键盘输入事件处理
    connect(m_remoteScreen, &RemoteScreenWidget::remoteKeyEvent,
            this, &Client::onRemoteKeyEvent);
}

Client::~Client()
{
    // 断开连接
    if (m_client->state() != QAbstractSocket::UnconnectedState) {
        m_client->close();
    }
    // 安全停止线程
    m_renderThread->requestInterruption();
    m_renderThread->wait();
    delete m_renderThread;
    delete m_remoteScreen;
    delete ui;
}

void sleep_ms(int milliseconds) {
    QThread::msleep(milliseconds);
}

void Client::onConnected()
{
    // ui->chat_frame->appendPlainText("✅ 连接服务器成功...");
    ui->conn_btn->setEnabled(false);
    ui->disconn_btn->setEnabled(true);

    // 发送欢迎消息
    m_client->sendTextMessage("客户端已连接!");

    m_connectionStatus->setStatus(LedIndicator::BlinkingGreen);

    m_client->sendTextMessage("hello, server");
}

void Client::onDisconnected()
{
    // ui->chat_frame->appendPlainText("❌ 与服务器断开连接...");
    ui->conn_btn->setEnabled(true);
    ui->disconn_btn->setEnabled(false);

    // 清理文件传输状态
    receivedFileData.clear();
    receivedFileSize = 0;
    expectedFileSize = 0;
    // ui->download_file->setEnabled(false);

    m_connectionStatus->setStatus(LedIndicator::BlinkingRed);
}

void Client::onTextMessageReceived(const QString& msg)
{
    // ui->chat_frame->appendPlainText("服务器: " + msg);
}

void Client::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    // ui->chat_frame->appendPlainText("❌ 连接错误: " + m_client.errorString());
    ui->conn_btn->setEnabled(true);
    ui->disconn_btn->setEnabled(false);
}

// 其实当服务端那边点击发送文件的时候，客户端就已经开始接收，然后使用数组保存起来，最后
// 客户端点击文件下载，保存到客户端指定的文件夹下面
void Client::onBinaryMessageReceived(const QByteArray &message)
{
    m_connectionStatus->setStatus(LedIndicator::BlinkingGreen);

    QDataStream stream(message);
    QString messageType;
    stream >> messageType;

    if(messageType == "SCREEN_FRAME"){
        processScreenFrame(message);
        return;
    }

    if(this -> is_send){
        // 判断客户端是请求还是下载文件（客户端首先请求，然后是下载，但是在客户端我们是合并在一个函数中）
        if (messageType == "FILE_REQUEST") {
            m_sendfile -> processFileRequest(message, m_client);
        } else if (messageType == "FILE_CHUNK_ACK") {
            // 处理文件块确认
            if(m_sendfile -> getIsFinished()){
                m_sendfile -> setIsFinished(false);
                this -> is_send = false;
                QMessageBox::information(this, "提示", "文件发送完成: " + m_sendfile -> getFileName());
                return;
            }
            m_sendfile -> sendFileChunk(m_client);
            ui -> upProgressBar->setValue(m_sendfile -> getProgress());
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

void Client::processFileHeader(const QByteArray &data)
{
    QDataStream stream(data);
    QString messageType;
    // 解析服务端发送的内容，由于服务端首先发送的是文件头部消息，所以这里首先解析头部消息
    stream >> messageType >> receivedFileName >> expectedFileSize;

    ui -> upProgressBar->setMaximum(expectedFileSize);

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

        m_client->sendBinaryMessage(ack);
    }
}

void Client::processFileChunk(const QByteArray &data)
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
        m_client->sendBinaryMessage(ack);

        // 检查是否接收完成
        if (receivedFileSize >= expectedFileSize) {
            // ui->chat_frame->appendPlainText("✅ 文件接收完成: " + receivedFileName);
            // ui->chat_frame->appendPlainText("💾 文件已缓存，点击下载按钮保存到本地");

            // 启用下载按钮
            // ui->download_file->setEnabled(true);
        }
    }
}

void Client::processScreenFrame(const QByteArray& message)
{
    QPixmap frame;

    QDataStream stream(message);
    QString messageType;
    QByteArray imageData;
    QSize imageSize;

    stream >> messageType >> imageData >> imageSize;
    if (!frame.loadFromData(imageData, "JPEG")) {
        qDebug() << "Failed to load image data";
        return;
    }

    // 安全地更新帧
    // 允许你通过方法名以字符串的形式来调用一个对象的成员函数。这种机制被称为“元对象调用”或“反射”，是 Qt 信号槽机制和动态调用的基础
    // 调用m_remoteScreen -> updateFrame()方法
    // Qt::QueuedConnection: 异步调用。方法会被包装成一个事件，放入目标对象所在线程的事件循环中，等待稍后执行。这是跨线程调用的安全方式
    // Q_ARG(QPixmap, frame)表示调用方法的参数类型以及参数值
    QMetaObject::invokeMethod(m_remoteScreen, "updateFrame",
                              Qt::QueuedConnection,
                              Q_ARG(QPixmap, frame));
}

void Client::onScreenVisibilityChanged(bool visible)
{
    // 可以在这里添加窗口显示/隐藏的额外逻辑
    if (visible) {
        m_remoteScreen->move(this->pos().x() + this->width() + 10,
                             this->pos().y());
    }
}

QString Client::getLocalIp()
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
        // ui->chat_frame->appendPlainText("❌ 未找到可用IPv4地址");
        // ui->chat_frame->appendPlainText("💡 使用回环地址: 127.0.0.1");
        ui->ip_text->setPlainText("127.0.0.1");
        return "127.0.0.1";
    }

    // ui->chat_frame->appendPlainText("✅ 推荐服务器地址: " + preferredIp);
    ui->ip_text->setPlainText(preferredIp);

    return preferredIp;
}

// 辅助函数：获取连接状态字符串
QString Client::getSocketStateString(QAbstractSocket::SocketState state)
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



void Client::on_conn_btn_clicked()
{
    const QString ip = ui->ip_text->toPlainText().trimmed();
    const int port = ui->port_box->value();

    // 输入验证
    if (ip.isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入服务器IP地址");
        return;
    }

    // 验证IP地址格式
    QHostAddress address;
    if (!address.setAddress(ip)) {
        QMessageBox::warning(this, "错误", "无效的IP地址格式");
        return;
    }

    // 验证端口范围
    if (port < 1 || port > 65535) {
        QMessageBox::warning(this, "错误", "端口号必须在1-65535之间");
        return;
    }

    // 检查是否已经连接
    if (m_client->state() != QAbstractSocket::UnconnectedState) {
        QMessageBox::information(this, "提示", "客户端已经连接或正在连接中");
        return;
    }

    // 构建WebSocket URL
    const QUrl websocket_url = QUrl(QString("ws://%1:%2").arg(ip).arg(port));
    if (!websocket_url.isValid()) {
        QMessageBox::warning(this, "错误", "无效的WebSocket URL");
        return;
    }

    // 开始连接
    m_client->open(websocket_url);
    // 正在连接服务器
    m_connectionStatus->setStatus(LedIndicator::BlinkingYellow);
}

void Client::on_disconn_btn_clicked()
{
    if (m_client->state() != QAbstractSocket::UnconnectedState) {
        m_client->close();
        ui->conn_btn->setEnabled(true);
        ui->disconn_btn->setEnabled(false);
    }
}

// 获取连接质量评估
QString Client::getConnectionQuality()
{
    // 这里可以根据需要实现更复杂的连接质量检测
    if (m_client->state() != QAbstractSocket::ConnectedState) {
        return "无连接";
    }

    // 简单返回"良好"，您可以扩展这个功能
    return "良好 ✅";
}

// 获取连接持续时间
QString Client::getConnectionDuration()
{
    // 如果需要跟踪连接时间，可以在连接成功时记录开始时间
    // 这里简单实现
    if (m_client->state() == QAbstractSocket::ConnectedState) {
        return "持续连接中";
    } else {
        return "未连接";
    }
}

void Client::on_search_btn_clicked()
{
    QString info;

    // 客户端连接状态
    info += "🔗 连接状态: " + getSocketStateString(m_client->state()) + "\n\n";

    // 服务器连接信息
    if (m_client->state() == QAbstractSocket::ConnectedState) {
        info += "🖥️ 服务器地址: " + m_client->peerAddress().toString() + "\n";
        info += "🚪 服务器端口: " + QString::number(m_client->peerPort()) + "\n";
        info += "📶 连接质量: " + getConnectionQuality() + "\n\n";
    }

    // 连接持续时间（如果需要的话）
    info += "⏱️ 连接时间: " + getConnectionDuration() + "\n";

    QMessageBox::information(this, "聊天信息", info);
}


void Client::on_exit_btn_clicked()
{
    // 优雅退出：先断开连接再关闭
    if (m_client->state() != QAbstractSocket::UnconnectedState) {
        m_client->close();
        // 等待短暂时间让连接正常关闭
        if (m_client->state() != QAbstractSocket::UnconnectedState) {
            m_client->abort(); // 强制中止
        }
    }
    this->close();
}

void Client::on_upload_btn_clicked(){
    QString filePath = QFileDialog::getOpenFileName(this, "选择要发送的文件",
                                                    QDir::homePath(), "所有文件 (*.*)");
    ui->upProgressBar->setValue(0);
    if (!filePath.isEmpty()) {
        m_sendfile -> sendfile_(filePath, m_client);
        this -> is_send = true;
    }
}


void Client::on_download_file_clicked()
{
    // 第一步：点击下载按钮首先向服务端进行请求
    if (m_client->state() != QAbstractSocket::ConnectedState) {
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
            // ui->chat_frame->appendPlainText("💾 文件已保存: " + savePath);
            QMessageBox::information(this, "提示", "文件保存至: " + savePath);
        } else {
            QMessageBox::warning(this, "错误", "无法保存文件: " + savePath);
        }
    }
}


void Client::on_control_btn_clicked()
{
    bool visible = m_remoteScreen->isVisible();
    m_remoteScreen->setVisible(!visible);
    ui->control_btn->setText(visible ? "显示屏幕" : "隐藏屏幕");
}

// 添加鼠标事件处理函数
void Client::onRemoteMouseEvent(QPoint position, Qt::MouseButton button, const QString &action)
{
    // 判断客户端和服务端是否处于连接状态
    if (m_client->state() != QAbstractSocket::ConnectedState) return;

    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);

    stream << QString("MOUSE_EVENT")
           << position
           << static_cast<int>(button)
           << action;
    // 将鼠标移动位置发送给服务端进行控制
    m_client->sendBinaryMessage(message);
}

// 添加键盘事件处理
void Client::onRemoteKeyEvent(int key, Qt::KeyboardModifiers modifiers, const QString &text, bool isPress)
{
    if (m_client->state() != QAbstractSocket::ConnectedState) return;

    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);

    stream << QString("KEY_EVENT")
           << key
           << static_cast<quint32>(modifiers)
           << text
           << isPress;

    m_client->sendBinaryMessage(message);
}

