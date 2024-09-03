#ifndef SDLFFMPEGVIDEOPLAYER_H
#define SDLFFMPEGVIDEOPLAYER_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class SDLFFmpegVideoPlayer;
}
QT_END_NAMESPACE

class SDLFFmpegVideoPlayer : public QMainWindow
{
    Q_OBJECT

public:
    SDLFFmpegVideoPlayer(QWidget *parent = nullptr);
    ~SDLFFmpegVideoPlayer();

private:
    Ui::SDLFFmpegVideoPlayer *ui;
};
#endif // SDLFFMPEGVIDEOPLAYER_H
