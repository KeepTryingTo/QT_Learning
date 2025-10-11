#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QObject>
#include <QMap>
#include <QMessageBox>
#include <QSoundEffect>

class SoundManager : public QObject
{
    Q_OBJECT

public:
    enum SoundType {
        PiecePlaced,
        Win,
        Lose,
        Draw,
        Click,
        Notification,
        Error,
        Surrender,
        Peace
    };

    explicit SoundManager(QObject *parent = nullptr);
    void playSound(SoundType type);
    void setVolume(int volume); // 0-100
    int volume() const;
    void setMuted(bool muted);
    bool isMuted() const;

private:
    QMap<SoundType, QSoundEffect*> m_sounds;
    int m_volume;
    bool m_muted;
};

#endif // SOUNDMANAGER_H
