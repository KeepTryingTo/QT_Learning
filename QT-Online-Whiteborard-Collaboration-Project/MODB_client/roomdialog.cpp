#include "roomdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

RoomDialog::RoomDialog(QWidget *parent)
    : QDialog(parent)
    , m_selectedRoomId("")
{
    setWindowTitle("选择房间");
    setFixedSize(400, 300);
    setupUI();
    loadRoomList();
}

RoomDialog::~RoomDialog()
{
}

void RoomDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 房间列表
    QLabel *listLabel = new QLabel("可用房间:", this);
    m_roomList = new QListWidget(this);
    connect(m_roomList, &QListWidget::itemClicked, this, &RoomDialog::onRoomSelected);

    // 创建新房间区域
    QLabel *createLabel = new QLabel("创建新房间:", this);
    m_roomNameEdit = new QLineEdit(this);
    m_roomNameEdit->setPlaceholderText("输入新房间名称");

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_createButton = new QPushButton("创建", this);
    m_joinButton = new QPushButton("加入", this);
    m_joinButton->setEnabled(false);

    connect(m_createButton, &QPushButton::clicked, this, &RoomDialog::onCreateRoomClicked);
    connect(m_joinButton, &QPushButton::clicked, this, &RoomDialog::onJoinRoomClicked);

    buttonLayout->addWidget(m_createButton);
    buttonLayout->addWidget(m_joinButton);

    mainLayout->addWidget(listLabel);
    mainLayout->addWidget(m_roomList);
    mainLayout->addWidget(createLabel);
    mainLayout->addWidget(m_roomNameEdit);
    mainLayout->addLayout(buttonLayout);
}

void RoomDialog::loadRoomList()
{
    // 这里应该从服务器获取房间列表
    // 暂时添加一些示例房间
    m_roomList->addItem("会议室 (001)");
    m_roomList->addItem("设计室 (002)");
    m_roomList->addItem("教学室 (003)");
}

void RoomDialog::onRoomSelected(QListWidgetItem *item)
{
    m_selectedRoomId = item->text().split("(").last().split(")").first();
    m_joinButton->setEnabled(true);
}

void RoomDialog::onCreateRoomClicked()
{
    if (m_roomNameEdit->text().isEmpty()) {
        m_roomNameEdit->setFocus();
        return;
    }
    m_selectedRoomId = ""; // 空字符串表示创建新房间
    accept();
}

void RoomDialog::onJoinRoomClicked()
{
    if (m_selectedRoomId.isEmpty()) {
        return;
    }
    accept();
}

QString RoomDialog::getRoomId() const
{
    return m_selectedRoomId;
}

QString RoomDialog::getRoomName() const
{
    return m_roomNameEdit->text();
}
