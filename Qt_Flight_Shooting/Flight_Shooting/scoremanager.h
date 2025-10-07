#ifndef SCOREMANAGER_H
#define SCOREMANAGER_H

#include <QObject>
#include <QGraphicsTextItem>

class ScoreManager : public QGraphicsTextItem
{
    Q_OBJECT

public:
    explicit ScoreManager(QGraphicsItem *parent = nullptr);
    // 增加当前分数
    void increaseScore(int points);
    int getScore() const;
    void reset();

signals:
    void sendScore(int points);
public slots:
    void updateDisplay();

private:
    int score;
};

#endif // SCOREMANAGER_H
