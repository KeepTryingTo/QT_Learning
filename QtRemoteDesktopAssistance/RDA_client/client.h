#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QFile>
#include <QDebug>
#include <QString>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QtWebSockets/QtWebSockets>

#include <iostream>

#include "ledindicator.h"
#include "sendfile.h"
#include "screencapturer.h"
#include "remotescreenwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Client;
}
QT_END_NAMESPACE

class Client : public QMainWindow
{
    Q_OBJECT

private slots:
    // 新增的槽函数
    QString getLocalIp();
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& msg);
    void onError(QAbstractSocket::SocketError error);

    QString getConnectionQuality();
    QString getConnectionDuration();
    QString getSocketStateString(QAbstractSocket::SocketState state);

    void onBinaryMessageReceived(const QByteArray &message);

    void processScreenFrame(const QByteArray& message);
    void onScreenVisibilityChanged(bool visible);

    // 远程控制鼠标事件
    void onRemoteMouseEvent(QPoint position, Qt::MouseButton button, const QString &action);
    void onRemoteKeyEvent(int key, Qt::KeyboardModifiers modifiers, const QString &text, bool isPress);

    void on_conn_btn_clicked();

    void on_disconn_btn_clicked();

    void on_search_btn_clicked();

    void on_upload_btn_clicked();

    void on_exit_btn_clicked();

    void on_download_file_clicked();

    void on_control_btn_clicked();

public:
    Client(QWidget *parent = nullptr);
    ~Client();

private:
    Ui::Client *ui;

    QWebSocket *m_client;

    QWidget *m_centralWidget;
    QHBoxLayout *m_mainLayout;
    LedIndicator *m_connectionStatus;

    sendFile * m_sendfile;
    bool is_send;

    // 文件传输相关变量
    QFile *receivedFile;
    qint64 expectedFileSize;
    qint64 receivedFileSize;
    QString receivedFileName;
    QByteArray receivedFileData;

    void processFileHeader(const QByteArray &data);
    void processFileChunk(const QByteArray &data);
    void saveReceivedFile();

    RemoteScreenWidget* m_remoteScreen;
    RenderThread* m_renderThread;
};
#endif // CLIENT_H
