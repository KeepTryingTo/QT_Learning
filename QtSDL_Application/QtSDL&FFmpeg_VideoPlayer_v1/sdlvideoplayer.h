#ifndef SDLVIDEOPLAYER_H
#define SDLVIDEOPLAYER_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class SDLVideoPlayer;
}
QT_END_NAMESPACE

class SDLVideoPlayer : public QMainWindow
{
    Q_OBJECT

public:
    SDLVideoPlayer(QWidget *parent = nullptr);
    ~SDLVideoPlayer();

private:
    Ui::SDLVideoPlayer *ui;
};
#endif // SDLVIDEOPLAYER_H
