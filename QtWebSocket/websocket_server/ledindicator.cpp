#include "ledindicator.h"

LedIndicator::LedIndicator(QWidget *parent)
    : QWidget(parent), m_status(Off), m_blinking(false) {
    setFixedSize(16, 16);

    m_blinkTimer = new QTimer(this);
    connect(m_blinkTimer, &QTimer::timeout, this, &LedIndicator::toggleBlink);
}
