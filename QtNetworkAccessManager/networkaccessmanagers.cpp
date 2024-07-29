#include "networkaccessmanagers.h"
#include "ui_networkaccessmanagers.h"

#include <QFile>
#include <QUrl>
#include <QMessageBox>

NetworkAccessManagers::NetworkAccessManagers(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NetworkAccessManagers)
{
    ui->setupUi(this);

    this -> Nmanager = new QNetworkAccessManager(this);
}

NetworkAccessManagers::~NetworkAccessManagers()
{
    delete ui;
}

void NetworkAccessManagers::on_btn_get_clicked()
{
    QString api = ui -> lineEdit -> text();//https://api.country.is/9.9.9.9
    QUrl APIurl(api);
    QNetworkRequest request(APIurl);

    this -> reply = this -> Nmanager -> get(request);

    QEventLoop loop;
    connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    loop.exec();

    if(reply -> error() == QNetworkReply::NoError){
        QString response = reply -> readAll();
        qDebug()<<"API Response: "<<response;
        ui -> plainTextEdit -> appendPlainText("API Response: " + response);
    }else{
        qDebug()<<"Error: "<<reply -> errorString();
        ui -> plainTextEdit -> appendPlainText("Error: " + reply -> errorString());
    }
    reply -> deleteLater();
}


void NetworkAccessManagers::on_btn_close_clicked()
{
    this -> close();
}

