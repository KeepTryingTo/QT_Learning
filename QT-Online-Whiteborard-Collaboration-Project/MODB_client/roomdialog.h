#ifndef ROOMDIALOG_H
#define ROOMDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

class RoomDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RoomDialog(QWidget *parent = nullptr);
    ~RoomDialog();

    QString getRoomId() const;
    QString getRoomName() const;

private slots:
    void onCreateRoomClicked();
    void onJoinRoomClicked();
    void onRoomSelected(QListWidgetItem *item);

private:
    QLineEdit *m_roomNameEdit;
    QListWidget *m_roomList;
    QPushButton *m_createButton;
    QPushButton *m_joinButton;

    QString m_selectedRoomId;

    void setupUI();
    void loadRoomList();
};

#endif // ROOMDIALOG_H
