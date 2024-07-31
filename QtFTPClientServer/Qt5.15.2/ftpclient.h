#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>

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

    QString  getLocalIp();
    QUrl applySetting(QString fileName);
    void uploadFile(QString path);
    void downloadFile(QString path);

private slots:
    void on_btn_open_clicked();

    void on_btn_close_clicked();

    void onUploadProgress(qint64 byteSent,qint64 totalBytes);
    void onFinished(QNetworkReply *);
    void onErrorOccurred(QNetworkReply::NetworkError error);
    void onDownloadProgress(qint64 bytesRecv, qint64 totalBytes);
    void onReadReady();

private:
    Ui::FTPClient *ui;
    QFile * file;

    QNetworkReply * reply;
    QNetworkAccessManager * networkAccessManager;
};
#endif // FTPCLIENT_H
