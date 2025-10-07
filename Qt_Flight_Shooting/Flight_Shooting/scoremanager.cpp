#include "scoremanager.h"
#include <QFont>

ScoreManager::ScoreManager(QGraphicsItem *parent)
    : QGraphicsTextItem(parent), score(0)
{
    setDefaultTextColor(Qt::white);
    setFont(QFont("Arial", 16));
    updateDisplay();
}

void ScoreManager::increaseScore(int points)
{
    score += points;
    updateDisplay();
    emit sendScore(points);
}

int ScoreManager::getScore() const
{
    return score;
}

void ScoreManager::reset()
{
    score = 0;
    updateDisplay();
}

void ScoreManager::updateDisplay()
{
    setPlainText(QString("得分: %1").arg(score));
}
