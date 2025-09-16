#include "screencapturer.h"
#include <QGuiApplication>
#include <QBuffer>
#include <QDataStream>
#include <QWebSocket>

ScreenCapturer::ScreenCapturer(QObject *parent)
    : QObject(parent),
    m_screen(QGuiApplication::primaryScreen()),
    m_captureTimer(new QTimer(this)),
    m_targetSocket(nullptr),
    m_frameRate(10),
    m_imageQuality(50),
    m_isCapturing(false)
{
    // 定时器间隔的捕获画面
    connect(m_captureTimer, &QTimer::timeout, this, &ScreenCapturer::captureScreen);
    // 时间捕获的间隔时间
    m_captureTimer->setInterval(1000 / m_frameRate);
}

ScreenCapturer::~ScreenCapturer()
{
    stopCapture();
}

void ScreenCapturer::setFrameRate(int fps)
{
    if (fps < 1 || fps > 60) {
        emit errorOccurred("帧率必须在1-60之间");
        return;
    }

    m_frameRate = fps;
    // 定时器的响应间隔时间
    m_captureTimer->setInterval(1000 / m_frameRate);
}

int ScreenCapturer::frameRate() const
{
    return m_frameRate;
}

void ScreenCapturer::setImageQuality(int quality)
{
    if (quality < 10 || quality > 100) {
        emit errorOccurred("图像质量必须在10-100之间");
        return;
    }

    m_imageQuality = quality;
}

int ScreenCapturer::imageQuality() const
{
    return m_imageQuality;
}

void ScreenCapturer::startCapture()
{
    if (m_isCapturing) {
        return;
    }

    if (!m_screen) {
        emit errorOccurred("无法获取屏幕对象");
        return;
    }
    // 启动捕获屏幕并设置开始捕获变量为true
    m_captureTimer->start();
    m_isCapturing = true;
}

void ScreenCapturer::stopCapture()
{
    m_captureTimer->stop();
    m_isCapturing = false;
}

bool ScreenCapturer::isCapturing() const
{
    return m_isCapturing;
}

void ScreenCapturer::setTargetSocket(QWebSocket *socket)
{
    m_targetSocket = socket;
}

void ScreenCapturer::captureScreen()
{
    if (!m_screen) {
        emit errorOccurred("屏幕对象无效");
        stopCapture();
        return;
    }

    try {
        // 捕获整个电脑屏幕
        QPixmap frame = m_screen->grabWindow(0);
        if (frame.isNull()) {
            emit errorOccurred("捕获屏幕失败");
            return;
        }
        // 捕获的帧
        emit frameCaptured(frame);
        sendFrame(frame);
    } catch (...) {
        emit errorOccurred("捕获屏幕时发生异常");
    }
}

void ScreenCapturer::sendFrame(const QPixmap &frame)
{
    if (!m_targetSocket || m_targetSocket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("目标socket未连接");
        return;
    }

    // 压缩图像为JPEG
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    // 将图像帧保存到frame中，然后发送给客户端
    if (!frame.save(&buffer, "JPEG", m_imageQuality)) {
        emit errorOccurred("图像压缩失败");
        return;
    }

    // 发送二进制消息
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream << QString("SCREEN_FRAME") << imageData << frame.size();

    m_targetSocket->sendBinaryMessage(message);
}
