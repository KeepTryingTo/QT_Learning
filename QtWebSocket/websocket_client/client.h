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

#include "ledindicator.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Client;
}
QT_END_NAMESPACE

class Client : public QMainWindow
{
    Q_OBJECT

public:
    Client(QWidget *parent = nullptr);
    ~Client();

private slots:
    void on_close_btn_clicked();

    void on_search_btn_clicked();

    void on_clear_btn_clicked();

    void on_send_btn_clicked();

    void on_exit_btn_clicked();

    void on_conn_btn_clicked();

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
    // 下载和请求
    void on_download_file_clicked();

private:
    Ui::Client *ui;

    QWebSocket m_client;

    QWidget *m_centralWidget;
    QHBoxLayout *m_mainLayout;
    LedIndicator *m_connectionStatus;

    // 文件传输相关变量
    QFile *receivedFile;
    qint64 expectedFileSize;
    qint64 receivedFileSize;
    QString receivedFileName;
    QByteArray receivedFileData;

    void processFileHeader(const QByteArray &data);
    void processFileChunk(const QByteArray &data);
    void saveReceivedFile();
};
#endif // CLIENT_H
