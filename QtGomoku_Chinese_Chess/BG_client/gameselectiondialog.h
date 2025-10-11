// GameSelectionDialog.h
#pragma once
#include <QDialog>
#include <QPushButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFile>
#include <QIcon>
#include <QCoreApplication>

class GameSelectionDialog : public QDialog {
    Q_OBJECT
public:
    explicit GameSelectionDialog(const QString &username, QWidget *parent = nullptr);
    QString getSelectedGame() const { return m_selectedGame; }
    QString getSelectedMode() const { return m_selectedMode; }

    void updateStartButton();

    QString getUserName(){
        return this -> m_username;
    }
    QString getGameName(){
        return this -> m_selectedGame;
    }
    QString getMode(){
        return this -> m_selectedMode;
    }

private slots:
    void onGameSelected(QAbstractButton *button);
    void onModeSelected(QAbstractButton *button);
    void onStartClicked();
    void onBackClicked();
    void onExitClicked();

private:
    QString m_username;
    QString m_selectedGame;
    QString m_selectedMode;

    QLabel *m_labelWelcome;
    QLabel *m_labelGame;
    QLabel *m_labelMode;

    QPushButton *m_btnGomoku;
    QPushButton *m_btnChess;
    QPushButton *m_btnPvP;
    QPushButton *m_btnPvE;
    QPushButton *m_btnStart;
    QPushButton *m_btnBack;
    QPushButton *m_btnExit;

    QButtonGroup *m_gameGroup;
    QButtonGroup *m_modeGroup;

    void setupUI();
    void applyStyle();
};
