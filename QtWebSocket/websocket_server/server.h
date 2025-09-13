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
#include <map>
#include <QFileDialog>

#include "ledindicator.h"

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

    void on_close_btn_clicked();

    void on_search_btn_clicked();

    void on_clear_btn_clicked();

    void on_send_btn_clicked();

    void on_exit_btn_clicked();

    void onClientDisconnected();
    void onNewConnection();
    QString getLocalIp();

    // 新增：发送文件按钮
    void onBinaryMessageReceived(const QByteArray &message);
    void on_send_file_clicked();

    QString getSocketStateString(QAbstractSocket::SocketState state);



private:
    Ui::Server *ui;
    QWebSocket* socket;
    QWebSocketServer* m_server;

    QWidget *m_centralWidget;
    QHBoxLayout *m_mainLayout;
    LedIndicator *m_connectionStatus;

    // 文件传输相关变量
    QFile *currentFile;
    qint64 fileSize;
    qint64 bytesWritten;
    QString currentFileName;
    QByteArray fileBuffer;

    void sendFile(const QString &filePath);
    void sendFileChunk();
    void processFileRequest(const QByteArray &data);
};
#endif // SERVER_H
