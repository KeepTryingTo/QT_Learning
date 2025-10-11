#include "server.h"
#include "ui_server.h"

Server::Server(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Server)
{
    ui->setupUi(this);

    QFile file(":/resources/windows.css");
    if (file.open(QIODevice::ReadOnly)) {
        QString strCss = file.readAll();
        this->setStyleSheet(strCss);
        file.close();
    } else {
        qDebug() << "无法加载CSS文件";
    }

    m_server = new QWebSocketServer("Gomoku Server", QWebSocketServer::NonSecureMode, this);

    QString ip = getLocalIp();
    ui -> ip_text->setText(ip);
    QHostAddress addr(ip);
    int port = ui -> port_box->value();

    std::cout<<"ip = "<<ip.toStdString()<<" port = "<<port<<std::endl;

    // 初始化UI状态
    ui->close_btn->setEnabled(false);
    ui->search_btn->setEnabled(true);
    ui->status_label->setText("服务器未启动");
    ui->status_label->setStyleSheet("color: red;");

    // 设置聊天框为只读
    ui->chat_frame->setReadOnly(true);

    // 显示初始信息
    appendToChatFrame("服务器程序已启动");
    appendToChatFrame("请点击\"监听\"按钮启动服务器");

    // 显示灯
    m_led = new LedIndicator(this);
    m_led->setStatus(LedIndicator::Off);
    // 添加到布局中
    layout()->addWidget(m_led);
    // 将指示灯设置为覆盖模式
    m_led->setParent(this);
    m_led->move(width() - m_led->width() - 10,height() - m_led->height() - 5);
}

Server::~Server()
{
    if (m_server) {
        m_server->close();
        delete m_server;
    }
    delete ui;
}

void delay(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

void Server::on_listen_btn_clicked()
{
    m_led -> setStatus(LedIndicator::BlinkingYellow);
    if (m_isListening) {
        appendToChatFrame("服务器已经在监听状态");
        return;
    }

    // 创建WebSocket服务器
    m_server = new QWebSocketServer("Gomoku Server", QWebSocketServer::NonSecureMode, this);

    // 获取端口号（可以从UI输入框获取，这里使用默认值）
    // m_serverPort = ui->port_edit->text().toUShort(); // 如果有端口输入框

    QHostAddress addr(ui -> ip_text->toPlainText());
    if (m_server->listen(addr, ui->port_box->value())) {
        m_isListening = true;

        connect(m_server, &QWebSocketServer::newConnection, this, &Server::onNewConnection);

        // 更新UI状态
        ui->listen_btn->setEnabled(false);
        ui->close_btn->setEnabled(true);
        ui->status_label->setText("服务器正在监听 - 端口: " + QString::number(ui->port_box->value()));
        ui->status_label->setStyleSheet("color: green;");

        // 获取本机IP地址
        QString ipAddress;
        QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
        for (const QHostAddress &address : ipAddressesList) {
            if (address != QHostAddress::LocalHost && address.toIPv4Address()) {
                ipAddress = address.toString();
                break;
            }
        }
        if (ipAddress.isEmpty()) {
            ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
        }

        appendToChatFrame("✓ 服务器启动成功");
        appendToChatFrame("监听地址: " + ipAddress);
        appendToChatFrame("监听端口: " + QString::number(ui->port_box->value()));
        appendToChatFrame("等待客户端连接...");

    } else {
        QString error = m_server->errorString();
        appendToChatFrame("✗ 服务器启动失败: " + error);
        QMessageBox::critical(this, "启动失败", "无法启动服务器:\n" + error);

        delete m_server;
        m_server = nullptr;
    }
}

void Server::on_close_btn_clicked()
{
    m_led->setStatus(LedIndicator::BlinkingRed);
    if (!m_isListening) {
        return;
    }

    // 断开所有客户端连接
    for (QWebSocket *client : m_clients.keys()) {
        client->close();
    }
    m_rooms.clear();
    m_userToRoom.clear();
    m_clients.clear();
    m_usernameToSocket.clear();

    // 关闭服务器
    m_server->close();
    delete m_server;
    m_server = nullptr;
    m_isListening = false;

    // 更新UI状态
    ui->listen_btn->setEnabled(true);
    ui->close_btn->setEnabled(false);
    ui->status_label->setText("服务器已关闭");
    ui->status_label->setStyleSheet("color: red;");

    appendToChatFrame("服务器已关闭");
    appendToChatFrame("所有客户端连接已断开");
}

void Server::on_search_btn_clicked()
{
    updateStatusDisplay();
}

void Server::on_exit_btn_clicked()
{
    // 先关闭服务器
    if (m_isListening) {
        on_close_btn_clicked();
    }

    m_rooms.clear();
    m_userToRoom.clear();

    // 退出程序
    qApp->quit();
}

void Server::onNewConnection()
{
    m_led->setStatus(LedIndicator::BlinkingGreen);
    QWebSocket *clientSocket = m_server->nextPendingConnection();
    if (!clientSocket) {
        return;
    }

    QString clientInfo = clientSocket->peerAddress().toString() + ":" +
                         QString::number(clientSocket->peerPort());

    appendToChatFrame("新的客户端连接: " + clientInfo);

    connect(clientSocket, &QWebSocket::textMessageReceived, this, &Server::onMessageReceived);
    connect(clientSocket, &QWebSocket::disconnected, this, &Server::onSocketDisconnected);

    m_clients.insert(clientSocket, "");

    updateOnlineUsersList();
    updateStatusDisplay();
}

void Server::onMessageReceived(const QString &message)
{
    // sender获得和当前连接的服务端socket
    QWebSocket *clientSocket = qobject_cast<QWebSocket*>(sender());
    if (!clientSocket) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        appendToChatFrame("收到无效的JSON消息");
        return;
    }

    QJsonObject jsonMessage = doc.object();
    processMessage(clientSocket, jsonMessage);
}

void Server::onSocketDisconnected()
{
    QWebSocket *clientSocket = qobject_cast<QWebSocket*>(sender());
    if (!clientSocket) {
        return;
    }
    // 获得客户端socket对应用户名
    QString username = m_clients.value(clientSocket);
    // 从用户列表中移除
    if (!username.isEmpty()) {
        appendToChatFrame("用户断开连接: " + username);

        // 从用户名映射中移除
        m_usernameToSocket.remove(username);

        // 广播用户下线通知
        QJsonObject logoutMessage;
        logoutMessage["type"] = static_cast<int>(MessageType::Logout);
        QJsonObject data;
        data["username"] = username;
        logoutMessage["data"] = data;
        // 广播通知另外一个客户端
        broadcastMessage(logoutMessage, clientSocket);
    } else {
        appendToChatFrame("匿名客户端断开连接");
    }

    // 移动对应的客户端socket信息
    m_clients.remove(clientSocket);
    clientSocket->deleteLater();

    if(m_clients.size() == 0){
        m_led->setStatus(LedIndicator::BlinkingRed);
    }

    updateOnlineUsersList();
    updateStatusDisplay();
}

void Server::processMessage(QWebSocket *client, const QJsonObject &message)
{
    // 判断收到客户端数据格式
    if (!message.contains("type") || !message.contains("data")) {
        appendToChatFrame("收到格式错误的消息");
        return;
    }

    // 获取对应数据类型，数据以及客户端socket对应的用户名
    MessageType type = static_cast<MessageType>(message["type"].toInt());
    QJsonObject data = message["data"].toObject();
    QString username = m_clients.value(client);

    switch (type) {
        case MessageType::CreateRoom: {
            if (username.isEmpty()) {
                sendError(client, "请先登录");
                return;
            }

            QString roomName = data["roomName"].toString();
            QString roomId = data["roomId"].toString();

            if (roomId.isEmpty() || roomName.isEmpty()) {
                sendError(client, "房间ID和名称不能为空");
                return;
            }
            // 当前是创建房间请求，首先先判断房间号是否存在；房间已经存在的话就将消息发送给客户端
            if (m_rooms.contains(roomId)) {
                sendError(client, "房间ID已存在");
                return;
            }
            // 再判断房间名称是否存在
            if (m_userToRoom.contains(username)) {
                sendError(client, "您已经在其他房间中");
                return;
            }

            // 创建新房间
            RoomInfo room;
            room.roomId = roomId;
            room.roomName = roomName;
            room.creator = username;
            room.members.insert(username);
            room.createTime = QDateTime::currentDateTime();

            m_rooms[roomId] = room;
            m_userToRoom[username] = roomId;

            // 发送创建成功响应
            QJsonObject response;
            response["type"] = static_cast<int>(MessageType::RoomCreated);
            QJsonObject responseData;
            responseData["roomId"] = roomId;
            responseData["roomName"] = roomName;
            response["data"] = responseData;

            QJsonDocument doc(response);
            client->sendTextMessage(doc.toJson(QJsonDocument::Compact));

            appendToChatFrame(username + " 创建了房间: " + roomName + " (ID: " + roomId + ")");
            break;
        }

        case MessageType::JoinRoom: {
            if (username.isEmpty()) {
                sendError(client, "请先登录");
                return;
            }

            QString roomId = data["roomId"].toString();

            // 首先判断要加入的房间是否存在；不存在就创建新的房间
            if (!m_rooms.contains(roomId)) {
                sendError(client, "房间不存在");
                return;
            }

            if (m_userToRoom.contains(username)) {
                sendError(client, "您已经在其他房间中");
                return;
            }

            RoomInfo &room = m_rooms[roomId];
            room.members.insert(username);
            m_userToRoom[username] = roomId;

            if (room.members.size() == 2) {
                QStringList members = room.members.values();
                QString firstPlayer = members[0];
                QString secondPlayer = members[1];

                std::cout<<"first player = "<<firstPlayer.toStdString()<<" second player = "<<secondPlayer.toStdString()<<std::endl;

                // 随机分配棋子颜色
                bool firstIsBlack = QRandomGenerator::global()->bounded(2) == 0;

                // 通知第一个玩家
                sendGameStartToUser(firstPlayer, secondPlayer,
                                    firstIsBlack ? PieceType::Black : PieceType::White);
                delay(30);// 延迟30毫秒
                // 通知第二个玩家
                sendGameStartToUser(secondPlayer, firstPlayer,
                                    firstIsBlack ? PieceType::White : PieceType::Black);
            }


            // 发送加入成功响应给请求者
            QJsonObject response;
            response["type"] = static_cast<int>(MessageType::RoomJoined);
            QJsonObject responseData;
            responseData["roomId"] = roomId;
            responseData["roomName"] = room.roomName;

            // 将房间的所有成员信息都响应给客户端
            QJsonArray membersArray;
            for (const QString &member : room.members) {
                membersArray.append(member);
            }
            responseData["members"] = membersArray;
            response["data"] = responseData;

            QJsonDocument doc(response);
            client->sendTextMessage(doc.toJson(QJsonDocument::Compact));

            // 广播给房间内其他成员，告诉有新的客户端加入了进来
            QJsonObject broadcastMsg;
            broadcastMsg["type"] = static_cast<int>(MessageType::Chat);
            QJsonObject broadcastData;
            broadcastData["message"] = username + " 加入了房间";
            broadcastData["sender"] = "系统";
            broadcastMsg["data"] = broadcastData;

            broadcastMessageToRoom(roomId, broadcastMsg, client);

            appendToChatFrame(username + " 加入了房间: " + room.roomName + " (ID: " + roomId + ")");
            break;
        }

        case MessageType::LeaveRoom: {
            if (username.isEmpty()) {
                sendError(client, "请先登录");
                return;
            }

            QString roomId = data["roomId"].toString();
            if (!m_userToRoom.contains(username) || m_userToRoom[username] != roomId) {
                sendError(client, "您不在此房间中");
                return;
            }

            //离开房间，需要删除对应用户信息
            if (m_rooms.contains(roomId)) {
                RoomInfo &room = m_rooms[roomId];
                room.members.remove(username);

                // 如果房间为空，删除房间
                if (room.members.isEmpty()) {
                    m_rooms.remove(roomId);
                    appendToChatFrame("房间 " + room.roomName + " 已被删除（无人）");
                }
            }
            // 将用户对应的房间信息删除
            m_userToRoom.remove(username);

            // 发送离开成功响应
            QJsonObject response;
            response["type"] = static_cast<int>(MessageType::RoomLeft);
            QJsonObject responseData;
            responseData["roomId"] = roomId;
            response["data"] = responseData;

            QJsonDocument doc(response);
            client->sendTextMessage(doc.toJson(QJsonDocument::Compact));

            appendToChatFrame(username + " 离开了房间: " + roomId);
            break;
        }

        case MessageType::RoomList: {
            QJsonArray roomsArray;

            for (const RoomInfo &room : m_rooms) {
                QJsonObject roomObj;
                roomObj["roomId"] = room.roomId;
                roomObj["roomName"] = room.roomName;
                roomObj["creator"] = room.creator;
                roomObj["memberCount"] = static_cast<int>(room.members.size());
                roomObj["createTime"] = room.createTime.toString(Qt::ISODate);

                roomsArray.append(roomObj);
            }

            QJsonObject response;
            response["type"] = static_cast<int>(MessageType::RoomList);
            QJsonObject responseData;
            responseData["rooms"] = roomsArray;
            response["data"] = responseData;

            QJsonDocument doc(response);
            client->sendTextMessage(doc.toJson(QJsonDocument::Compact));
            break;
        }
        case MessageType::Login: {
            // 如果是登录类型的话，获取用户名
            QString newUsername = data["username"].toString();
            if (newUsername.isEmpty()) {
                sendError(client, "用户名不能为空");
                return;
            }

            if (m_usernameToSocket.contains(newUsername)) {
                sendError(client, "用户名已被占用");
                return;
            }

            // 注册用户
            m_clients[client] = newUsername;
            m_usernameToSocket[newUsername] = client;

            appendToChatFrame("用户登录: " + newUsername);

            // 发送登录成功响应
            QJsonObject response;
            response["type"] = static_cast<int>(MessageType::Login);
            QJsonObject responseData;
            responseData["status"] = "success";
            responseData["username"] = newUsername;
            response["data"] = responseData;

            QJsonDocument doc(response);
            // 响应客户端登录成功
            client->sendTextMessage(doc.toJson(QJsonDocument::Compact));

            // 广播用户上线通知
            QJsonObject userJoinedMessage;
            userJoinedMessage["type"] = static_cast<int>(MessageType::Login);
            QJsonObject joinedData;
            joinedData["username"] = newUsername;
            joinedData["action"] = "joined";
            userJoinedMessage["data"] = joinedData;

            // 广播给其他的客户端该用户上线了
            broadcastMessage(userJoinedMessage, client);

            updateOnlineUsersList();
            break;
        }

        case MessageType::Move: {
            // 对方落棋子了
            if (username.isEmpty()) {
                sendError(client, "请先登录");
                return;
            }

            int x = data["x"].toInt();
            int y = data["y"].toInt();
            int piece = data["piece"].toInt();

            appendToChatFrame(username + " 落子: (" + QString::number(x) + ", " +
                              QString::number(y) + "), 棋子: " +
                              (piece == 1 ? "黑棋" : "白棋"));

            // 添加发送者信息并广播
            QString roomId = m_userToRoom[username];
            data["sender"] = username;
            QJsonObject moveMessage;
            moveMessage["type"] = static_cast<int>(MessageType::Move);
            moveMessage["data"] = data;

            // 只转发给同房间的其他玩家
            broadcastMessageToRoom(roomId, moveMessage, client);
            break;
        }

        case MessageType::Chat: {
            // 如果是聊天的话
            if (username.isEmpty()) {
                sendError(client, "请先登录");
                return;
            }

            QString chatMessage = data["message"].toString();
            appendToChatFrame(username + " 说: " + chatMessage);

            // 添加发送者信息并广播
            data["sender"] = username;
            QJsonObject chatMsg;
            chatMsg["type"] = static_cast<int>(MessageType::Chat);
            chatMsg["data"] = data;
            // 将聊天的消息发送给其他客户端
            broadcastMessage(chatMsg);
            break;
        }

        case MessageType::GameStart: {
            // 如果是游戏开始但是没有登录的话提醒用户登录游戏
            if (username.isEmpty()) {
                sendError(client, "请先登录");
                return;
            }

            QString opponent = data["opponent"].toString();
            appendToChatFrame(username + " 开始与 " + opponent + " 对战");

            data["sender"] = username;
            QJsonObject gameStartMessage;
            gameStartMessage["type"] = static_cast<int>(MessageType::GameStart);
            gameStartMessage["data"] = data;

            // 将游戏开始的消息广播其他的客户端
            broadcastMessage(gameStartMessage);
            break;
        }

        case MessageType::GameEnd: {
            if (username.isEmpty()) {
                sendError(client, "请先登录");
                return;
            }

            // 将游戏获胜的消息告诉给其他的客户端
            QString winner = data["winner"].toString();
            appendToChatFrame("游戏结束 - 获胜者: " + winner);

            data["sender"] = username;
            QJsonObject gameEndMessage;
            gameEndMessage["type"] = static_cast<int>(MessageType::GameEnd);
            gameEndMessage["data"] = data;
            broadcastMessage(gameEndMessage);
            break;
        }
        case MessageType::UserList: {
            // 确保所有挂起的登录操作都已完成
            QCoreApplication::processEvents();
            // 获取在线用户列表
            QStringList onlineUsers = getOnlineUsers();

            QJsonArray usersArray;
            for (const QString &user : onlineUsers) {
                usersArray.append(user);
            }

            // 发送用户列表响应
            QJsonObject response;
            response["type"] = static_cast<int>(MessageType::UserListResponse);
            QJsonObject responseData;
            responseData["users"] = usersArray;
            response["data"] = responseData;

            QJsonDocument doc(response);
            client->sendTextMessage(doc.toJson(QJsonDocument::Compact));
            break;
        }
        // 在现有的case语句后面添加这个新的case
        case MessageType::StatsUpdate: {
            if (username.isEmpty()) {
                sendError(client, "请先登录");
                return;
            }

            // 提取统计数据
            QString targetUsername = data["username"].toString();
            int score = data["score"].toInt();
            int wins = data["wins"].toInt();
            int losses = data["losses"].toInt();
            int draws = data["draws"].toInt();
            int streak = data["streak"].toInt();

            // 记录统计更新
            appendToChatFrame("收到用户统计更新: " + targetUsername +
                              " - 分数: " + QString::number(score) +
                              ", 胜/负/平: " + QString::number(wins) + "/" +
                              QString::number(losses) + "/" + QString::number(draws) +
                              ", 连胜: " + QString::number(streak));

            // storeUserStats(targetUsername, score, wins, losses, draws, streak);

            // 可以选择广播给所有客户端或特定客户端
            QJsonObject response;
            response["type"] = static_cast<int>(MessageType::StatsUpdate);
            response["data"] = data;

            // 广播给所有客户端
            broadcastMessage(response);

            break;
        }
        case MessageType::ResumeStart: {
            QString roomId = data["roomId"].toString();

            // 首先判断要加入的房间是否存在；不存在就创建新的房间
            if (!m_rooms.contains(roomId)) {
                sendError(client, "房间不存在");
                return;
            }

            RoomInfo &room = m_rooms[roomId];
            QStringList members = room.members.values();
            QString firstPlayer = members[0];
            QString secondPlayer = members[1];

            QWebSocket *client1 = m_usernameToSocket[firstPlayer];
            QWebSocket *client2 = m_usernameToSocket[secondPlayer];

            QJsonObject message;
            message["type"] = static_cast<int>(MessageType::ResumeStart);
            QJsonObject data;
            data["username"] = firstPlayer;
            message["data"] = data;

            QJsonDocument doc(message);
            client1->sendTextMessage(doc.toJson(QJsonDocument::Compact));

            delay(30);
            client2->sendTextMessage(doc.toJson(QJsonDocument::Compact));

            break;
        }
        case MessageType::GIVE_UP: {

            QString username = data["username"].toString();
            QString roomId = data["roomId"].toString();

            std::cout<<"GIVE UP info = "<<username.toStdString()<<std::endl;

            QJsonObject message;
            message["type"] = static_cast<int>(MessageType::GIVE_UP);
            QJsonObject data;
            data["username"] = username;
            message["data"] = data;

            RoomInfo &room = m_rooms[roomId];
            QStringList members = room.members.values();
            QString firstPlayer = members[0];
            QString secondPlayer = members[1];

            QWebSocket *client1 = firstPlayer == username ? m_usernameToSocket[firstPlayer] : m_usernameToSocket[secondPlayer];

            broadcastMessageToRoom(roomId, message, client1);
            break;
        }
        default:
            appendToChatFrame("收到未知类型的消息");
            break;
    }
}

void Server::sendGameStartToUser(const QString &username, const QString &opponent, PieceType pieceType) {
    if (!m_usernameToSocket.contains(username)) {
        return;
    }

    QWebSocket *client = m_usernameToSocket[username];

    std::cout<<"client id = "<<client<<std::endl;

    QJsonObject message;
    message["type"] = static_cast<int>(MessageType::GameStart);
    QJsonObject data;
    data["opponent"] = opponent;
    data["myPieceType"] = static_cast<int>(pieceType);
    data["opponentPieceType"] = static_cast<int>(pieceType == PieceType::Black ? PieceType::White : PieceType::Black);
    message["data"] = data;

    QJsonDocument doc(message);
    client->sendTextMessage(doc.toJson(QJsonDocument::Compact));

    appendToChatFrame("游戏开始: " + username + " 执" +
                      (pieceType == PieceType::Black ? "黑棋" : "白棋") +
                      " vs " + opponent);
}

// 根据房间ID号广播其他客户端信息
void Server::broadcastMessageToRoom(const QString &roomId, const QJsonObject &message, QWebSocket *exclude)
{
    std::cout<<"broadcastMessageToRoom"<<" Room id = "<<roomId.toStdString()<<std::endl;
    if (!m_rooms.contains(roomId)) {
        return;
    }

    const RoomInfo &room = m_rooms[roomId];
    QJsonDocument doc(message);
    QString jsonString = doc.toJson(QJsonDocument::Compact);

    for (const QString &username : room.members) {
        if (m_usernameToSocket.contains(username)) {
            QWebSocket *client = m_usernameToSocket[username];
            if (client != exclude && client->state() == QAbstractSocket::ConnectedState) {
                client->sendTextMessage(jsonString);
            }
        }
    }
}

void Server::broadcastMessage(const QJsonObject &message, QWebSocket *exclude)
{
    QJsonDocument doc(message);
    QString jsonString = doc.toJson(QJsonDocument::Compact);

    int sentCount = 0;
    // 将消息广播给其他的客户端
    for (QWebSocket *client : m_clients.keys()) {
        if (client != exclude && client->state() == QAbstractSocket::ConnectedState) {
            client->sendTextMessage(jsonString);
            sentCount++;
        }
    }

    // 可选：显示广播统计
    if (sentCount > 0) {
        // appendToChatFrame("消息已广播给 " + QString::number(sentCount) + " 个客户端");
    }
}

// 将发生错误的消息发送给其他的客户端
void Server::sendError(QWebSocket *client, const QString &errorMessage)
{
    QJsonObject errorMsg;
    errorMsg["type"] = static_cast<int>(MessageType::Error);

    QJsonObject data;
    data["error"] = errorMessage;
    errorMsg["data"] = data;

    QJsonDocument doc(errorMsg);
    client->sendTextMessage(doc.toJson(QJsonDocument::Compact));

    appendToChatFrame("发送错误消息: " + errorMessage);
}

void Server::updateStatusDisplay()
{
    QString status;

    if (m_isListening) {
        status = "服务器状态: 运行中\n";
        status += "监听端口: " + QString::number(ui->port_box->value()) + "\n";
        status += "客户端数量: " + QString::number(m_clients.size()) + "\n";
        status += "在线用户: " + QString::number(m_usernameToSocket.size()) + "\n";

        if (!m_usernameToSocket.isEmpty()) {
            status += "用户列表:\n";
            for (const QString &user : m_usernameToSocket.keys()) {
                status += "  • " + user + "\n";
            }
        }
    } else {
        status = "服务器状态: 已停止\n";
    }

    // 显示在状态标签或消息框中
    QMessageBox::information(this, "服务器状态", status);
}

// 将信息输出到聊天框中
void Server::appendToChatFrame(const QString &message)
{
    QDateTime currentTime = QDateTime::currentDateTime();
    QString timestamp = currentTime.toString("hh:mm:ss");

    QString formattedMessage = "[" + timestamp + "] " + message + "\n";

    // 添加到聊天框
    ui->chat_frame->appendPlainText(formattedMessage);

    // 自动滚动到底部
    QTextCursor cursor = ui->chat_frame->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->chat_frame->setTextCursor(cursor);
}

void Server::updateOnlineUsersList()
{
    // 如果有专门的在线用户列表控件
    // ui->online_users_list->clear();
    // ui->online_users_list->addItems(m_usernameToSocket.keys());

    // 或者在状态栏显示
    QString userCount = "在线用户: " + QString::number(m_usernameToSocket.size());
    // statusBar()->showMessage(userCount);
}

// 获取所有在线用户
QStringList Server::getOnlineUsers() const
{
    return m_usernameToSocket.keys();
}

// 将消息发送给指定的用户
bool Server::sendToUser(const QString &username, const QJsonObject &message)
{
    if (!m_usernameToSocket.contains(username)) {
        return false;
    }

    QWebSocket *client = m_usernameToSocket.value(username);
    if (client && client->state() == QAbstractSocket::ConnectedState) {
        QJsonDocument doc(message);
        client->sendTextMessage(doc.toJson(QJsonDocument::Compact));
        return true;
    }

    return false;
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



