#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class videoPlayer;
}
QT_END_NAMESPACE

class videoPlayer : public QMainWindow
{
    Q_OBJECT

public:
    videoPlayer(QWidget *parent = nullptr);
    ~videoPlayer();

private:
    Ui::videoPlayer *ui;
};
#endif // VIDEOPLAYER_H
