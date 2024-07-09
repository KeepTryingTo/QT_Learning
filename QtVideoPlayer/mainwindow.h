#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QGraphicsVideoItem>
#include <QFileDialog>
#include <QGraphicsScale>
#include <QMessageBox>
#include <QTime>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private slots:
    void onStateChanged(QMediaPlayer::PlaybackState state);
    void onDurationChanged(qint64 duration);
    void onPositionChanged(qint64 position);

    void on_btnOpen_clicked();

    void on_btnStart_clicked();

    void on_btnStop_clicked();

    void on_btnEnd_clicked();

    void on_videoVolumn_valueChanged(int value);

    void on_progressBar_sliderMoved(int position);

private:
    QMediaPlayer *player;
    QGraphicsVideoItem * videoItems;

    //记录播放的时间
    QString durationTime;
    QString psitiontime;

private:
    Ui::MainWindow *ui;
    QString fileName;
    QAudioOutput * audioOutput;
    QTime * time;
};
#endif // MAINWINDOW_H
