#include "player.h"

Player::Player(const QString& name, PieceType pieceType, bool isAI)
    : m_name(name),
    m_pieceType(pieceType),
    m_score(0),
    m_isAI(isAI),
    m_streak(0),
    m_totalWins(0),
    m_totalLosses(0),
    m_totalDraws(0), // 新增
    m_currentStreak(0) {}

QString Player::getName() const {
    return m_name;
}

PieceType Player::getPieceType() const {
    return m_pieceType;
}

int Player::getScore() const {
    return m_score;
}

bool Player::isAI() const {
    return m_isAI;
}

void Player::setAI(bool ai) {
    m_isAI = ai;
}

void Player::resetScore() {
    m_score = 0;
}

void Player::setPieceType(PieceType piece) {
    m_pieceType = piece;
}

void Player::setName(const QString& name) {
    m_name = name;
}

// Player.cpp 实现
double Player::getWinRate(int totalGames) const {
    return totalGames > 0 ? (m_totalWins * 100.0 / totalGames) : 0;
}

int Player::getStreak() const {
    return m_streak;
}

int Player::getLevel() const {
    return m_score / 100; // 假设每100分升一级
}

int Player::getTotalWins() const {
    return m_totalWins;
}

int Player::getTotalLosses() const {
    return m_totalLosses;
}

// void Player::addWin() {
//     m_totalWins++;
//     if (m_currentStreak >= 0) {
//         m_currentStreak++;
//     } else {
//         m_currentStreak = 1; // 连败结束，开始连胜
//     }
// }

// void Player::addLoss() {
//     m_totalLosses++;
//     if (m_currentStreak <= 0) {
//         m_currentStreak--;
//     } else {
//         m_currentStreak = -1; // 连胜结束，开始连败
//     }
// }

void Player::copyFrom(const Player&player){
    if(&player == this)return;
    if(player.getName() != this->m_name){
        std::cout<<"different user name"<<std::endl;
        return;
    }
    this->m_name = player.m_name;
    this->m_score = player.m_score;
    this->m_streak = player.m_streak;
    this->m_totalDraws = player.m_totalDraws;
    this->m_totalLosses = player.m_totalLosses;
    this->m_totalWins = player.m_totalWins;
    this->m_isAI = player.m_isAI;
}
Player::Player(const Player&player){
    this->m_name = player.m_name;
    this->m_score = player.m_score;
    this->m_streak = player.m_streak;
    this->m_totalDraws = player.m_totalDraws;
    this->m_totalLosses = player.m_totalLosses;
    this->m_totalWins = player.m_totalWins;
    this->m_isAI = player.m_isAI;
}
Player&Player::operator=(const Player&player){
    if(&player == this)return *this;
    this->m_name = player.m_name;
    this->m_score = player.m_score;
    this->m_streak = player.m_streak;
    this->m_totalDraws = player.m_totalDraws;
    this->m_totalLosses = player.m_totalLosses;
    this->m_totalWins = player.m_totalWins;
    this->m_isAI = player.m_isAI;

    return *this;
}

void Player::resetStats() {
    m_totalWins = 0;
    m_totalLosses = 0;
    m_currentStreak = 0;
    m_score = 0;
}

int Player::getTotalDraws() const {
    return m_totalDraws;
}

void Player::resetStreak() {
    m_currentStreak = 0;
}

void Player::updateStreak(bool won) {
    if (won) {
        // 如果赢了，连胜数增加（如果是正数）或重置为1（如果是负数）
        if (m_currentStreak >= 0) {
            m_currentStreak++;
        } else {
            m_currentStreak = 1;
        }
    } else {
        // 如果输了，连败数增加（如果是负数）或重置为-1（如果是正数）
        if (m_currentStreak <= 0) {
            m_currentStreak--;
        } else {
            m_currentStreak = -1;
        }
    }
}

void Player::addScore(int points) {
    if (points > 0) {
        // 连胜奖励：每连胜3场额外加1分
        int bonus = (m_currentStreak >= 3) ? (m_currentStreak / 3) : 0;
        m_score += points + bonus;
    }
}

//
void Player::addWin() {
    m_totalWins++;
    updateStreak(true); // 更新连胜
}

void Player::addLoss() {
    m_totalLosses++;
    updateStreak(false); // 更新连败
}

void Player::addDraw() {
    m_totalDraws++;
    resetStreak(); // 平局中断连胜/连败
}
