#include "networkmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket)
    , m_isConnected(false)
{
    connect(m_webSocket, &QWebSocket::connected, this, &NetworkManager::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &NetworkManager::onTextMessageReceived);
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &NetworkManager::onError);
}

NetworkManager::~NetworkManager()
{
    disconnectFromServer();
    delete m_webSocket;
}

bool NetworkManager::connectToServer(const QUrl &url)
{
    if (m_isConnected) {
        disconnectFromServer();
    }

    m_serverUrl = url;
    m_webSocket->open(url);
    return true;
}

void NetworkManager::disconnectFromServer()
{
    if (m_isConnected) {
        logout();
        m_webSocket->close();
    }
}

void NetworkManager::sendMessage(const NetworkMessage &message)
{
    if (!m_isConnected) {
        qWarning() << "Not connected to server";
        return;
    }

    QByteArray jsonData = message.toJson();
    m_webSocket->sendTextMessage(QString::fromUtf8(jsonData));
}

void NetworkManager::login(const QString &username)
{
    m_username = username;
    NetworkMessage message = createLoginMessage(username);
    sendMessage(message);
}

void NetworkManager::logout()
{
    if (!m_username.isEmpty()) {
        NetworkMessage message;
        message.type = MessageType::Logout;
        QJsonObject data;
        data["username"] = m_username;
        message.data = data;
        sendMessage(message);
    }
}

void NetworkManager::sendMove(int x, int y, PieceType piece)
{
    NetworkMessage message = createMoveMessage(x, y, piece);
    sendMessage(message);
}

void NetworkManager::sendGameStart(const QString &opponent)
{
    NetworkMessage message = createGameStartMessage(opponent);
    sendMessage(message);
}

void NetworkManager::sendGameEnd(const QString &winner)
{
    NetworkMessage message = createGameEndMessage(winner);
    sendMessage(message);
}

void NetworkManager::sendChatMessage(const QString &message)
{
    NetworkMessage msg = createChatMessage(message);
    sendMessage(msg);
}

bool NetworkManager::isConnected() const
{
    return m_isConnected;
}

QString NetworkManager::getUsername() const
{
    return m_username;
}

void NetworkManager::onConnected()
{
    m_isConnected = true;
    emit connected();
    qDebug() << "Connected to server:" << m_serverUrl.toString();
}

void NetworkManager::onDisconnected()
{
    m_isConnected = false;
    emit disconnected();
    qDebug() << "Disconnected from server";
}

void NetworkManager::onTextMessageReceived(const QString &message)
{
    QByteArray jsonData = message.toUtf8();
    NetworkMessage networkMessage = NetworkMessage::fromJson(jsonData);

    if (networkMessage.type == MessageType::Invalid) {
        qWarning() << "Invalid message received";
        return;
    }

    emit messageReceived(networkMessage);

    // 分发特定类型的消息
    switch (networkMessage.type) {
    case MessageType::Move: {
        int x = networkMessage.data["x"].toInt();
        int y = networkMessage.data["y"].toInt();
        PieceType piece = static_cast<PieceType>(networkMessage.data["piece"].toInt());
        emit moveReceived(x, y, piece);
        break;
    }
    case MessageType::GameStart: {
        QString opponent = networkMessage.data["opponent"].toString();
        emit gameStartReceived(opponent);
        break;
    }
    case MessageType::GameEnd: {
        QString winner = networkMessage.data["winner"].toString();
        emit gameEndReceived(winner);
        break;
    }
    case MessageType::Chat: {
        QString chatMsg = networkMessage.data["message"].toString();
        emit chatMessageReceived(chatMsg);
        break;
    }
    case MessageType::Error: {
        QString error = networkMessage.data["error"].toString();
        emit errorOccurred(error);
        break;
    }
    default:
        break;
    }
}

void NetworkManager::onError(QAbstractSocket::SocketError error)
{
    QString errorMsg = m_webSocket->errorString();
    emit errorOccurred(errorMsg);
    qWarning() << "WebSocket error:" << errorMsg;
}
void NetworkManager::sendStatsUpdate(const QString &username, int score,
                                     int wins, int losses, int draws, int streak) {
    NetworkMessage message;
    message.type = MessageType::StatsUpdate;
    QJsonObject data;
    data["username"] = username;
    data["score"] = score;
    data["wins"] = wins;
    data["losses"] = losses;
    data["draws"] = draws;
    data["streak"] = streak;
    message.data = data;

    sendMessage(message);
}
