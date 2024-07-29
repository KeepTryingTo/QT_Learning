#ifndef NETWORKACCESSMANAGERS_H
#define NETWORKACCESSMANAGERS_H

#include <QMainWindow>

#include <QNetworkAccessManager>
#include <QNetworkAddressEntry>
#include <QNetworkReply>
#include <QDesktopServices>

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
    void on_btn_get_clicked();

    void on_btn_close_clicked();

private:
    Ui::NetworkAccessManagers *ui;

    QNetworkAccessManager * Nmanager;
    QNetworkReply * reply;
};
#endif // NETWORKACCESSMANAGERS_H
