#ifndef SDLAUDIOPLAYER_H
#define SDLAUDIOPLAYER_H

#include <QMainWindow>

#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QSize>
#include <QStyle>
#include <QPixmap>
#include <QMap>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class SDLAudioPlayer;
}
QT_END_NAMESPACE

class SDLAudioPlayer : public QMainWindow
{
    Q_OBJECT

public:
    SDLAudioPlayer(QWidget *parent = nullptr);
    ~SDLAudioPlayer();
    void updateDuration(qint64 duration);
    void showMetaDataDialog();

    void initMixer();
    void initSDL();
    void initTabel();

    void addTableWidgetItem(QString filePath);

private slots:
    void on_btn_close_clicked();

    void on_btn_open_file_clicked();

    void on_btn_seek_backward_clicked();

    void on_btn_stop_clicked();

    void on_btn_start_clicked();

    void on_btn_pause_clicked();

    void on_btn_seek_feedward_clicked();

    void on_btn_muted_clicked();

    void on_slider_time_valueChanged(int value);

    void on_slider_volumn_valueChanged(int value);

    void positionChanged(qint64 progress);

    void on_spinBox_valueChanged(int arg1);

    void on_btn_clear_clicked();

private:
    Ui::SDLAudioPlayer *ui;

    QString fileName;
    qint64 Mduration;
    bool is_muted = false;
    bool isPaused = false;
    double currPlayPosition = 0;
    int rows = 0;

    Mix_Music * music;
    SDL_Event e;

    QMap<int,QString>map;
};
#endif // SDLAUDIOPLAYER_H
