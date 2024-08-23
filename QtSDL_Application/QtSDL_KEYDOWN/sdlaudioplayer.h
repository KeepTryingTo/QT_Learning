#ifndef SDLAUDIOPLAYER_H
#define SDLAUDIOPLAYER_H

#include <QMainWindow>

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

private:
    Ui::SDLAudioPlayer *ui;
};
#endif // SDLAUDIOPLAYER_H
