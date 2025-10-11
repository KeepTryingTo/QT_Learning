#include "ledindicator.h"

LedIndicator::LedIndicator(QWidget *parent)
    : QWidget(parent), m_status(Off), m_blinking(false) {
    setFixedSize(16, 16);

    // 创建定时器以及绑定定时器
    m_blinkTimer = new QTimer(this);
    // 定时器触发之后就向槽函数发送消息，本项目是500ms触发一次
    connect(m_blinkTimer, &QTimer::timeout, this, &LedIndicator::toggleBlink);
}
