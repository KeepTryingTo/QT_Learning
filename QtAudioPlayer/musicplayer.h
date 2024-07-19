#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QMainWindow>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QMediaPlayer>
#include <QTime>
#include <QAudioOutput>


QT_BEGIN_NAMESPACE
namespace Ui {
class MusicPlayer;
}
QT_END_NAMESPACE

class MusicPlayer : public QMainWindow
{
    Q_OBJECT

public:
    MusicPlayer(QWidget *parent = nullptr);
    ~MusicPlayer();

private slots:
    void on_btn_open_file_clicked();

    void on_btn_start_clicked();

    void on_btn_pause_clicked();

    void on_btn_close_clicked();

    void onStateChanged(QMediaPlayer::PlaybackState state);

    void on_slider_volume_valueChanged(int value);

private:
    Ui::MusicPlayer *ui;

    QFile file;
    QString fileName;
    QString durationTime;
    qint64 total_times;

    QMediaPlayer * audioPlayer;
    QTime * time;
    QAudioOutput * audioOutput;
};
#endif // MUSICPLAYER_H
