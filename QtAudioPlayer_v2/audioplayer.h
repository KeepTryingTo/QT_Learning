#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QFile>
#include <QAudioOutput>

QT_BEGIN_NAMESPACE
namespace Ui {
class AudioPlayer;
}
QT_END_NAMESPACE

class AudioPlayer : public QMainWindow
{
    Q_OBJECT

public:
    AudioPlayer(QWidget *parent = nullptr);
    ~AudioPlayer();

    void updateDuration(qint64 duration);

private slots:
    void on_btn_open_file_clicked();

    void on_btn_close_clicked();

    void on_btn_muted_clicked();

    void on_btn_seek_backward_clicked();

    void on_btn_stop_clicked();

    void on_btn_player_clicked();

    void on_btn_pause_clicked();

    void on_btn_seek_feedward_clicked();

    void on_progress_volumn_valueChanged(int value);

    void on_progress_time_valueChanged(int value);

    void durationChanged(qint64 duration);
    void positionChanged(qint64 progress);

private:
    Ui::AudioPlayer *ui;

    QString fileName;
    bool is_muted = false;
    qint64 Mduration;

    QMediaPlayer * audioPlayer;
    QAudioOutput * audioOutput;
};
#endif // AUDIOPLAYER_H
