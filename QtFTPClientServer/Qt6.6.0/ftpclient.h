#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <QMainWindow>

#include <curl/curl.h>
#include <QFileInfo>
#include <QHostAddress>
#include <QHostInfo>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class FTPClient;
}
QT_END_NAMESPACE

class FTPClient : public QMainWindow
{
    Q_OBJECT

public:
    FTPClient(QWidget *parent = nullptr);
    ~FTPClient();

    QString getLocalIp();

    void downloadFile(QString path);
    void uploadFile(QString path);

    QString applySetting(QString fileName);

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
    static size_t ReadCallback(void *ptr, size_t size, size_t nmemb, void *userdata);

    bool ftpUpload_and_download(const QString &url, const QString &username, const QString &password, const QString &localFilepath);

public slots:
    void on_btn_open_clicked();

    void on_btn_close_clicked();

private:
    Ui::FTPClient *ui;

    QFile * file;
    qint64 fileSize;
    bool isUpload = true;
};
#endif // FTPCLIENT_H
