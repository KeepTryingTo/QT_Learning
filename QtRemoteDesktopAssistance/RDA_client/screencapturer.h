#ifndef SCREEN_CAPTURER_H
#define SCREEN_CAPTURER_H

#include <QObject>
#include <QScreen>
#include <QPixmap>
#include <QTimer>
#include <QByteArray>

class QWebSocket;

class ScreenCapturer : public QObject
{
    Q_OBJECT

public:
    explicit ScreenCapturer(QObject *parent = nullptr);
    ~ScreenCapturer();

    // 设置/获取帧率
    void setFrameRate(int fps);
    int frameRate() const;

    // 设置/获取图像质量
    void setImageQuality(int quality);
    int imageQuality() const;

    // 开始/停止捕获
    void startCapture();
    void stopCapture();
    bool isCapturing() const;

    // 设置目标socket
    void setTargetSocket(QWebSocket *socket);

signals:
    void frameCaptured(const QPixmap &frame);
    void errorOccurred(const QString &error);

private slots:
    void captureScreen();

private:
    void sendFrame(const QPixmap &frame);

    QScreen *m_screen;
    QTimer *m_captureTimer;
    QWebSocket *m_targetSocket;
    int m_frameRate;
    int m_imageQuality;
    bool m_isCapturing;
};

#endif // SCREEN_CAPTURER_H
