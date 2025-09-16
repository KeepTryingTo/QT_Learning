#ifndef SERVER_H
#define SERVER_H

#include <QMainWindow>
#include <QFile>
#include <QDebug>
#include <QString>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QtWebSockets/QtWebSockets>
#include <QtWebSockets/QWebSocketServer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QHostInfo>

#include <QFileDialog>

#include <map>
#include <iostream>

#include "ledindicator.h"
#include "sendfile.h"
#include "screencapturer.h"
#include "remotemousecontroller.h"
#include "remotekeyboardcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Server;
}
QT_END_NAMESPACE

class Server : public QMainWindow
{
    Q_OBJECT

public:
    Server(QWidget *parent = nullptr);
    ~Server();

private slots:
    void on_listen_btn_clicked();

    void onClientDisconnected();
    void onNewConnection();
    QString getLocalIp();

    // 新增：发送文件按钮
    void onBinaryMessageReceived(const QByteArray &message);
    QString getSocketStateString(QAbstractSocket::SocketState state);
    void processFileChunk(const QByteArray &data);
    void processFileHeader(const QByteArray &data);
    QString getConnectionQuality();
    QString getConnectionDuration();

    void on_exit_btn_clicked();

    void on_close_btn_clicked();

    void on_sendfile_btn_clicked();

    void on_download_file_btn_clicked();

    void on_search_btn_clicked();

    void on_start_screen_shared_clicked();

    void on_close_shared_btn_clicked();

    void on_frame_ratio_slider_sliderMoved(int position);

    void on_quality_slider_sliderMoved(int position);

private:
    // 鼠标事件处理
    void processMouseEvent(const QByteArray &data);
    // 键盘输入事件处理
    void processKeyEvent(const QByteArray &data);

private:
    Ui::Server *ui;

    QWebSocket* socket;
    QWebSocketServer* m_server;

    // 指示灯
    QWidget *m_centralWidget;
    QHBoxLayout *m_mainLayout;
    LedIndicator *m_connectionStatus;

    // 文件传输
    sendFile * m_sendfile;
    QFile *receivedFile;
    qint64 expectedFileSize;
    qint64 receivedFileSize;
    QString receivedFileName;
    QByteArray receivedFileData;

    bool is_send;

    //实时传输画面
    ScreenCapturer* m_screenCapturer;

    //鼠标控制
    RemoteMouseController *m_mouseController;

    // 键盘输入
    RemoteKeyboardController *m_keyboardController;
};
#endif // SERVER_H
