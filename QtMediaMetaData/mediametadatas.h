#ifndef MEDIAMETADATAS_H
#define MEDIAMETADATAS_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QMediaMetaData>

QT_BEGIN_NAMESPACE
namespace Ui {
class MediaMetaDataS;
}
QT_END_NAMESPACE

class MediaMetaDataS : public QMainWindow
{
    Q_OBJECT

public:
    MediaMetaDataS(QWidget *parent = nullptr);
    ~MediaMetaDataS();

private slots:

    void OnMediaStatusChanged(QMediaPlayer::MediaStatus status);

    void on_btn_open_file_clicked();

    void on_btn_close_clicked();

private:
    Ui::MediaMetaDataS *ui;

    QString fileName;

    QMediaPlayer *player;
    QMediaMetaData * metaData;
};
#endif // MEDIAMETADATAS_H
