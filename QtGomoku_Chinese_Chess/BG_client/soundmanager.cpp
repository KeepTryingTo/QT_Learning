#include "soundmanager.h"

SoundManager::SoundManager(QObject *parent)
    : QObject(parent), m_volume(50), m_muted(false)
{
    // 初始化音效
    m_sounds[PiecePlaced] = new QSoundEffect(this);
    m_sounds[PiecePlaced]->setSource(QUrl(":/../images/chess_move.wav"));

    m_sounds[Win] = new QSoundEffect(this);
    m_sounds[Win]->setSource(QUrl(":/../images/win.wav"));

    m_sounds[Lose] = new QSoundEffect(this);
    m_sounds[Lose]->setSource(QUrl(":/../images/lose.wav"));

    m_sounds[Click] = new QSoundEffect(this);
    m_sounds[Click]->setSource(QUrl(":/../images/click.wav"));

    m_sounds[Notification] = new QSoundEffect(this);
    m_sounds[Notification]->setSource(QUrl(":/../images/notification.wav"));

    m_sounds[Error] = new QSoundEffect(this);
    m_sounds[Error]->setSource(QUrl(":/../images/error.wav"));

    // 设置音量
    setVolume(m_volume);
}

void SoundManager::playSound(SoundType type)
{
    if (m_muted || !m_sounds.contains(type)) {
        return;
    }

    m_sounds[type]->play();
}

void SoundManager::setVolume(int volume)
{
    m_volume = qBound(0, volume, 100);
    qreal linearVolume = m_volume / 100.0;

    for (QSoundEffect *sound : m_sounds) {
        sound->setVolume(linearVolume);
    }
}

int SoundManager::volume() const
{
    return m_volume;
}

void SoundManager::setMuted(bool muted)
{
    m_muted = muted;
}

bool SoundManager::isMuted() const
{
    return m_muted;
}
