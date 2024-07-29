#ifndef NETWORKACCESSMANAGERS_H
#define NETWORKACCESSMANAGERS_H

#include <QMainWindow>

#include <QNetworkAccessManager>
#include <QNetworkAddressEntry>
#include <QNetworkReply>
#include <QDesktopServices>

#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QUrl>
#include <QDir>
#include <QEventLoop>

QT_BEGIN_NAMESPACE
namespace Ui {
class NetworkAccessManagers;
}
QT_END_NAMESPACE

class NetworkAccessManagers : public QMainWindow
{
    Q_OBJECT

public:
    NetworkAccessManagers(QWidget *parent = nullptr);
    ~NetworkAccessManagers();

private slots:
    void on_btn_close_clicked();

    void on_btn_save_clicked();

    void on_btn_download_clicked();

    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onFinished();
    void onReadyRead();

private:
    Ui::NetworkAccessManagers *ui;

    QString fileDir;

    QNetworkAccessManager * Nmanager;
    QNetworkReply * reply;
    QFile * fp;
    QEventLoop * loop;
};
#endif // NETWORKACCESSMANAGERS_H
