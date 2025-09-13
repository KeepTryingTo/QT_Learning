#include "client.h"
#include "ui_client.h"
#include <QFile>
#include <QMessageBox>
#include <QUrl>

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

    this -> getLocalIp();
    // 重要：禁用代理，直接连接
    m_client.setProxy(QNetworkProxy::NoProxy);
    // 连接信号槽
    connect(&m_client, &QWebSocket::connected, this, &Client::onConnected);
    connect(&m_client, &QWebSocket::disconnected, this, &Client::onDisconnected);
    connect(&m_client, &QWebSocket::textMessageReceived, this, &Client::onTextMessageReceived);
    connect(&m_client, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &Client::onError);

    // 连接二进制消息信号
    connect(&m_client, &QWebSocket::binaryMessageReceived, this, &Client::onBinaryMessageReceived);
}

Client::~Client()
{
    // 断开连接
    if (m_client.state() != QAbstractSocket::UnconnectedState) {
        m_client.close();
    }
    delete ui;
}

void sleep_ms(int milliseconds) {
    QThread::msleep(milliseconds);
}

void Client::onConnected()
{
    ui->chat_frame->appendPlainText("✅ 连接服务器成功...");
    ui->conn_btn->setEnabled(false);
    ui->close_btn->setEnabled(true);
    ui->send_btn->setEnabled(true);

    // 发送欢迎消息
    m_client.sendTextMessage("客户端已连接!");

    m_connectionStatus->setStatus(LedIndicator::BlinkingGreen);

}

void Client::onDisconnected()
{
    ui->chat_frame->appendPlainText("❌ 与服务器断开连接...");
    ui->conn_btn->setEnabled(true);
    ui->close_btn->setEnabled(false);
    ui->send_btn->setEnabled(false);

    // 清理文件传输状态
    receivedFileData.clear();
    receivedFileSize = 0;
    expectedFileSize = 0;
    ui->download_file->setEnabled(false);

    m_connectionStatus->setStatus(LedIndicator::BlinkingRed);
}

void Client::onTextMessageReceived(const QString& msg)
{
    ui->chat_frame->appendPlainText("服务器: " + msg);
}

void Client::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    ui->chat_frame->appendPlainText("❌ 连接错误: " + m_client.errorString());
    ui->conn_btn->setEnabled(true);
    ui->close_btn->setEnabled(false);
    ui->send_btn->setEnabled(false);
}

// 其实当服务端那边点击发送文件的时候，客户端就已经开始接收，然后使用数组保存起来，最后
// 客户端点击文件下载，保存到客户端指定的文件夹下面
void Client::onBinaryMessageReceived(const QByteArray &message)
{
    QDataStream stream(message);
    QString messageType;
    stream >> messageType;

    // 接收下载文件
    if (messageType == "FILE_HEADER") {
        processFileHeader(message);
    } else if (messageType == "FILE_CHUNK") {
        processFileChunk(message);
    }
}

void Client::processFileHeader(const QByteArray &data)
{
    QDataStream stream(data);
    QString messageType;
    // 解析服务端发送的内容，由于服务端首先发送的是文件头部消息，所以这里首先解析头部消息
    stream >> messageType >> receivedFileName >> expectedFileSize;

    if (messageType == "FILE_HEADER") {
        receivedFileSize = 0;
        receivedFileData.clear();

        ui->chat_frame->appendPlainText("📥 开始接收文件: " + receivedFileName +
                                        " (" + QString::number(expectedFileSize) + " bytes)");

        // 确认接收
        QByteArray ack;
        QDataStream ackStream(&ack, QIODevice::WriteOnly);
        // 接收服务端发送的文件头部信息之后，客户端回应响应内容
        ackStream << QString("FILE_ACK") << receivedFileName;

        m_client.sendBinaryMessage(ack);
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
        ui->chat_frame->appendPlainText("📊 接收进度: " + QString::number(progress) + "%");

        // 发送确认
        QByteArray ack;
        QDataStream ackStream(&ack, QIODevice::WriteOnly);
        // 接收服务端发送的文件头部信息之后，客户端将正式请求下载内容
        ackStream << QString("FILE_CHUNK_ACK");
        m_client.sendBinaryMessage(ack);

        // 检查是否接收完成
        if (receivedFileSize >= expectedFileSize) {
            ui->chat_frame->appendPlainText("✅ 文件接收完成: " + receivedFileName);
            ui->chat_frame->appendPlainText("💾 文件已缓存，点击下载按钮保存到本地");

            // 启用下载按钮
            ui->download_file->setEnabled(true);
        }
    }
}

QString Client::getLocalIp()
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
    if (m_client.state() != QAbstractSocket::UnconnectedState) {
        QMessageBox::information(this, "提示", "客户端已经连接或正在连接中");
        return;
    }

    // 构建WebSocket URL
    const QUrl websocket_url = QUrl(QString("ws://%1:%2").arg(ip).arg(port));
    if (!websocket_url.isValid()) {
        QMessageBox::warning(this, "错误", "无效的WebSocket URL");
        return;
    }

    ui->chat_frame->appendPlainText("🔗 正在连接服务器: " + websocket_url.toString());

    // 开始连接
    m_client.open(websocket_url);
    // 正在连接服务器
    m_connectionStatus->setStatus(LedIndicator::BlinkingYellow);
}

void Client::on_close_btn_clicked()
{
    if (m_client.state() != QAbstractSocket::UnconnectedState) {
        m_client.close();
        ui->chat_frame->appendPlainText("🛑 已主动断开连接");
        ui->conn_btn->setEnabled(true);
        ui->close_btn->setEnabled(false);
        ui->send_btn->setEnabled(false);
    }
}

// 获取连接质量评估
QString Client::getConnectionQuality()
{
    // 这里可以根据需要实现更复杂的连接质量检测
    if (m_client.state() != QAbstractSocket::ConnectedState) {
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
    if (m_client.state() == QAbstractSocket::ConnectedState) {
        return "持续连接中";
    } else {
        return "未连接";
    }
}

void Client::on_search_btn_clicked()
{
    QString info;

    // 客户端连接状态
    info += "🔗 连接状态: " + getSocketStateString(m_client.state()) + "\n\n";

    // 服务器连接信息
    if (m_client.state() == QAbstractSocket::ConnectedState) {
        info += "🖥️ 服务器地址: " + m_client.peerAddress().toString() + "\n";
        info += "🚪 服务器端口: " + QString::number(m_client.peerPort()) + "\n";
        info += "📶 连接质量: " + getConnectionQuality() + "\n\n";
    }

    // 统计客户端和服务端分别发送的消息数量
    QString chatText = ui->chat_frame->toPlainText();
    int totalMessages = chatText.count("\n") + 1;
    int sentMessages = chatText.count("客户端: ");
    int receivedMessages = chatText.count("服务端: ");
    int systemMessages = totalMessages - sentMessages - receivedMessages;

    info += "📊 消息统计\n";
    info += "────────────\n";
    info += "💬 总消息数: " + QString::number(totalMessages) + "\n";
    info += "📤 发送消息: " + QString::number(sentMessages) + "\n";
    info += "📥 接收消息: " + QString::number(receivedMessages) + "\n";
    info += "⚙️ 系统消息: " + QString::number(systemMessages) + "\n\n";

    // 连接持续时间（如果需要的话）
    info += "⏱️ 连接时间: " + getConnectionDuration() + "\n";

    QMessageBox::information(this, "聊天信息", info);
}

void Client::on_clear_btn_clicked()
{
    ui->chat_frame->clear();
}

void Client::on_send_btn_clicked()
{
    // 检查连接状态
    if (m_client.state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "错误", "未连接到服务器，无法发送消息");
        return;
    }

    QString msg = ui->input_text->toPlainText().trimmed();
    if (msg.isEmpty()) {
        QMessageBox::information(this, "提示", "消息内容不能为空");
        return;
    }

    // 发送消息
    m_client.sendTextMessage(msg);
    ui->chat_frame->appendPlainText("客户端: " + msg);
    ui->input_text->clear();
}

void Client::on_exit_btn_clicked()
{
    // 优雅退出：先断开连接再关闭
    if (m_client.state() != QAbstractSocket::UnconnectedState) {
        m_client.close();
        // 等待短暂时间让连接正常关闭
        if (m_client.state() != QAbstractSocket::UnconnectedState) {
            m_client.abort(); // 强制中止
        }
    }
    this->close();
}



void Client::on_download_file_clicked()
{
    // 第一步：点击下载按钮首先向服务端进行请求
    if (m_client.state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "错误", "未连接到服务器，无法请求文件");
        return;
    }

    // QString fileName = QInputDialog::getText(this, "请求文件", "请输入文件名:");
    // if (!fileName.isEmpty()) {
    //     // 发送文件请求
    //     QByteArray request;
    //     QDataStream stream(&request, QIODevice::WriteOnly);
    //     stream << QString("FILE_REQUEST") << fileName << qint64(0); // 文件大小未知

    //     m_client.sendBinaryMessage(request);
    //     ui->chat_frame->appendPlainText("📤 已请求文件: " + fileName);
    // }

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

