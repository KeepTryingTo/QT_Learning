#include "client.h"
#include "ui_client.h"

Client::Client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Client)
    , m_webSocketManager(new WebSocketManager(this))
    , m_isOnlineMode(false)

{
    ui->setupUi(this);
    ip = "127.0.0.1";
    port = 8080;

    // 创建指示灯
    m_connectionStatus = new LedIndicator(this);
    statusBar()->addPermanentWidget(m_connectionStatus);  // 添加到状态栏右侧


    // 加载CSS样式
    QFile file(":/resources/windows.css");
    if (file.open(QIODevice::ReadOnly)) {
        QString strCss = file.readAll();
        this->setStyleSheet(strCss);
        file.close();
    } else {
        qDebug() << "无法加载CSS文件";
    }

    is_full = false;

    // 设置工具栏图标（可以使用Qt内置图标或自定义图标）,因为我的系统中没有就自己下载图标并从资源文件中加载
    // ui->pen_btn->setIcon(QIcon::fromTheme("draw-brush")); // 或使用自定义图标
    // ui->line_btn->setIcon(QIcon::fromTheme("draw-line"));
    // ui->rect_btn->setIcon(QIcon::fromTheme("draw-rectangle"));
    // ui->elli_btn->setIcon(QIcon::fromTheme("draw-ellipse"));
    // ui->text_btn->setIcon(QIcon::fromTheme("draw-text"));
    // ui->zoom_big_btn->setIcon(QIcon::fromTheme("zoom-in"));
    // ui->zoom_small_btn->setIcon(QIcon::fromTheme("zoom-out"));

    // 设置按钮为可选中状态
    ui->pen_btn->setCheckable(true);
    ui->line_btn->setCheckable(true);
    ui->rect_btn->setCheckable(true);
    ui->elli_btn->setCheckable(true);
    ui->text_btn->setCheckable(true);

    // 创建按钮组，确保只有一个工具被选中
    QButtonGroup *toolGroup = new QButtonGroup(this);
    toolGroup->addButton(ui -> pen_btn);
    toolGroup->addButton(ui -> line_btn);
    toolGroup->addButton(ui -> rect_btn);
    toolGroup->addButton(ui -> elli_btn);
    toolGroup->addButton(ui -> text_btn);


    // 初始化场景和视图
    QGraphicsScene *scene = new QGraphicsScene(this);
    ui->whiteBoard->setScene(scene);

    // 创建绘图工具
    m_drawingTool = new DrawingTool(scene, this);


    // 设置视图的鼠标事件转发
    ui->whiteBoard->setMouseTracking(true);
    if(ui -> whiteBoard -> viewport()){
        std::cout<<"ui -> whiteBoard -> viewport"<<std::endl;
    }
    Q_ASSERT(ui->whiteBoard->viewport() != nullptr);
    // 开启事件过滤器，鼠标事件得以被捕获和处理，从而调用对应的绘图方法
    ui -> whiteBoard -> viewport()->installEventFilter(this);

    // 颜色提取器
    connect(ui->color_btn, &QToolButton::clicked, this, &Client::onColorBtnClicked);


    // 初始化文件管理器
    m_fileManager = new FileManager(this);

    // 连接文件管理器信号
    connect(m_fileManager, &FileManager::fileOpened, this, [this](const QString &fileName) {
        setWindowTitle("白板 - " + QFileInfo(fileName).fileName());
    });

    connect(m_fileManager, &FileManager::fileSaved, this, [this](const QString &fileName) {
        setWindowTitle("白板 - " + QFileInfo(fileName).fileName());
        QMessageBox::information(this, "保存成功", "文件已保存");
    });

    connect(m_fileManager, &FileManager::errorOccurred, this, [this](const QString &message) {
        QMessageBox::warning(this, "错误", message);
    });

    // 连接修改信号
    connect(m_drawingTool, &DrawingTool::contentModified, this, [this]() {
        // 只要修改了内容之后，需要标注当前是已修改状态
        if(!(m_fileManager -> getModified())){
            // 第一次更改的时候添加*，后面继续更改就不需要添加*了
            setWindowTitle(windowTitle() + " *");
        }
        m_fileManager->setModified(true);
    });

    // 连接菜单信号（如果还没有自动连接）
    connect(ui->newFile, &QAction::triggered, this, &Client::on_newFile);
    connect(ui->openFile, &QAction::triggered, this, &Client::on_openFile);
    connect(ui->saveFile, &QAction::triggered, this, &Client::on_saveFile);
    connect(ui->saveAs, &QAction::triggered, this, &Client::on_saveAsFile);
    connect(ui->exportFile, &QAction::triggered, this, &Client::on_exportImage);
    connect(ui->zoom, &QAction::triggered, this, [this](){
        ui->whiteBoard->scale(1.2, 1.2);
    });

    // 初始化网格状态
    m_showGrid = true;  // 默认显示网格
    // 连接网格切换动作
    connect(ui->gridView, &QAction::triggered, this, &Client::on_toggleGridAction);
    updateGrid();

    // 在client.cpp中连接信号
    connect(m_fileManager, &FileManager::gridStateChanged, this, [this](bool showGrid) {
        m_showGrid = showGrid;
        updateGrid();

        // 更新菜单项文本
        if (m_showGrid) {
            ui->gridView->setText("隐藏网格");
        } else {
            ui->gridView->setText("显示网格");
        }
    });

    // 连接清除,撤销和重做动作
    connect(ui->clear, &QAction::triggered, this, &Client::on_clearAction);
    // 在Client构造函数中添加调试连接
    connect(ui->undo, &QAction::triggered, this, [this]() {
        qDebug() << "撤销按钮被点击";
        try {
            m_drawingTool->undo();
        } catch (const std::exception& e) {
            qCritical() << "撤销操作崩溃:" << e.what();
            QMessageBox::warning(this, "错误", QString("撤销操作失败: %1").arg(e.what()));
        } catch (...) {
            qCritical() << "撤销操作发生未知崩溃";
            QMessageBox::warning(this, "错误", "撤销操作发生未知错误");
        }
    });

    connect(ui->redo, &QAction::triggered, this, [this]() {
        qDebug() << "重做按钮被点击";
        try {
            this -> on_redoAction();
        } catch (const std::exception& e) {
            qCritical() << "重做操作崩溃:" << e.what();
            QMessageBox::warning(this, "错误", QString("重做操作失败: %1").arg(e.what()));
        } catch (...) {
            qCritical() << "重做操作发生未知崩溃";
            QMessageBox::warning(this, "错误", "重做操作发生未知错误");
        }
    });

    // 连接场景清除信号
    connect(m_drawingTool, &DrawingTool::sceneCleared, this, [this]() {
        // 清除白板也记录当前为修改状态
        if(!(m_fileManager -> getModified())){
            // 第一次更改的时候添加*，后面继续更改就不需要添加*了
            setWindowTitle(windowTitle() + " *");
        }
        m_fileManager->setSceneCleared();
    });

    // 在关闭程序之前，首先询问一下是否保存文件
    connect(ui ->exit, &QAction::triggered, this, [this, scene]() -> bool{
        if(m_fileManager -> getModified()){
            QMessageBox::StandardButton ret = QMessageBox::warning(nullptr, "白板",
                                                                   "文档已被修改。\n是否保存更改？",
                                                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

            // 保存或者不保存修改内容
            if (ret == QMessageBox::Save) {
                m_fileManager -> saveFile(scene);
            } else if (ret == QMessageBox::Cancel) {
                return false;
            }
        }
        if(this -> m_webSocketManager->isConnected()){
            delete this -> m_webSocketManager;
        }
        this -> close();
        return true;
    });


    // 布局拉伸
    // 设置主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(ui->centralwidget);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(2);

    // 确保function_btn显示在whiteBoard之上
    ui->function_btn->raise(); // 将按钮区域提到最前面

    // 将工具栏和画布添加到主布局
    mainLayout->addWidget(ui->function_btn);
    mainLayout->addWidget(ui->whiteBoard);

    // 设置拉伸因子
    mainLayout->setStretch(0, 0); // 工具栏不拉伸
    mainLayout->setStretch(1, 1); // 画布拉伸

    // 设置大小策略
    ui->function_btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->whiteBoard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 设置function_btn的最小高度，确保可见
    ui->function_btn->setMinimumHeight(91); // 根据您的按钮大小调整


    // 初始化帮助管理器
    m_helpManager = new HelpManager(this);

    // 连接菜单信号
    connect(ui->about, &QAction::triggered, this, &Client::onAboutTriggered);
    connect(ui->document, &QAction::triggered, this, &Client::onHelpTriggered);
    connect(ui->version, &QAction::triggered, this, &Client::onVersionTriggered);
    connect(ui->update, &QAction::triggered, this, &Client::onCheckForUpdates);

    // 连接更新检查信号
    connect(m_helpManager, &HelpManager::updateAvailable, this, &Client::onUpdateAvailable);

    // 启动时自动检查更新（静默模式）
    QTimer::singleShot(30000, this, [this]() {m_helpManager->checkForUpdates(true);});

    // 连接菜单信号
    connect(ui->connServer, &QAction::triggered, this, &Client::onConnectToServerClicked);
    connect(ui->joinRoom, &QAction::triggered, this, &Client::onJoinRoomClicked);
    connect(ui->disconnect, &QAction::triggered, this, &Client::disconnectClient);


    // 连接DrawingTool的信号
    connect(m_drawingTool, &DrawingTool::drawingOperationCreated,this, &Client::onDrawingOperationCreated);
    connect(m_drawingTool, &DrawingTool::clearSceneRequested,this, &Client::onClearSceneRequested);
    connect(m_drawingTool, &DrawingTool::undoRequested,this, &Client::onUndoRequested);
    connect(m_drawingTool, &DrawingTool::redoRequested,this, &Client::onRedoRequested);
    connect(m_drawingTool, &DrawingTool::contentModified, this, [this]() {
        // 只要修改了内容之后，需要标注当前是已修改状态
        if(!(m_fileManager -> getModified())){
            // 第一次更改的时候添加*，后面继续更改就不需要添加*了
            setWindowTitle(windowTitle() + " *");
        }
        m_fileManager->setModified(true);
    });

    // 连接WebSocketManager的信号
    connect(m_webSocketManager, &WebSocketManager::undoRequestReceived,this, &Client::onUndoRequestReceived);
    connect(m_webSocketManager, &WebSocketManager::redoRequestReceived,this, &Client::onRedoRequestReceived);
    connect(m_webSocketManager, &WebSocketManager::connected, this, &Client::onConnectedToServer);
    connect(m_webSocketManager, &WebSocketManager::disconnected, this, &Client::onDisconnectedFromServer);
    connect(m_webSocketManager, &WebSocketManager::connectionError, this, &Client::onConnectionError);
    connect(m_webSocketManager, &WebSocketManager::drawingOperationReceived, this, &Client::onDrawingOperationReceived);
    connect(m_webSocketManager, &WebSocketManager::clearSceneReceived, this, &Client::onClearSceneReceived);
    connect(m_webSocketManager, &WebSocketManager::roomError, this, &Client::recvError);
    connect(m_webSocketManager, &WebSocketManager::roomCreated, this, [this](const QString &roomId, const QString & roomName){
        this -> m_currentRoomId = roomId;
    });


    // 初始化聊天对话框
    m_chatDialog = new ChatDialog(this);
    m_chatDialog->setUserName(m_userName);
    connect(m_chatDialog, &ChatDialog::messageSent, this, &Client::onChatMessageSent);
    // 连接聊天菜单项
    connect(ui->chat_frame, &QAction::triggered, this, &Client::onChatActionTriggered);
    // 连接WebSocketManager的聊天消息信号
    connect(m_webSocketManager, &WebSocketManager::chatMessageReceived,this, &Client::onChatMessageReceived);


    // 初始化用户列表
    setupUserListWidget();
    // 连接用户信息菜单项
    connect(ui->user_info, &QAction::triggered, this, &Client::onUserInfoActionTriggered);
    // 连接用户列表相关信号
    connect(m_webSocketManager, &WebSocketManager::clientListReceived, this, &Client::onClientListReceived);
    connect(m_webSocketManager, &WebSocketManager::userJoined, this, &Client::onUserJoined);
    connect(m_webSocketManager, &WebSocketManager::userLeft, this, &Client::onUserLeft);

    connect(m_userListDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (!visible && m_userListVisible) {
            m_userListVisible = false;
            ui->user_info->setText("显示用户列表");
        }
    });


    // 连接新的信号
    connect(m_drawingTool, &DrawingTool::undoRequestedWithData, this, &Client::onUndoRequestedWithData);
    connect(m_drawingTool, &DrawingTool::redoRequestedWithData, this, &Client::onRedoRequestedWithData);
}

Client::~Client()
{
    delete ui;
}

// 在client.cpp中实现
void Client::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // 确保画布视图适应新的窗口大小
    QGraphicsScene* scene = ui->whiteBoard->scene();
    if (scene) {
        // 调整视图大小以适应窗口
        ui->whiteBoard->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

        // 窗口大小变化时重新生成网格
        updateGrid();
    }
}

bool Client::eventFilter(QObject *watched, QEvent *event)
{

    if (watched == ui->whiteBoard->viewport()) {
        // 首先检查事件类型
        if (event->type() != QEvent::MouseButtonPress &&
            event->type() != QEvent::MouseMove &&
            event->type() != QEvent::MouseButtonRelease) {
            return QMainWindow::eventFilter(watched, event);
        }

        // 白板上鼠标绘图事件、将通用的 QEvent对象强制转换为具体的 QMouseEvent类型
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        // 创建一个 QGraphicsSceneMouseEvent对象，并用原始事件的类型（如 MouseButtonPress、MouseMove、MouseButtonRelease）初始化它
        QGraphicsSceneMouseEvent sceneEvent(event->type());

        // 设置产生此事件的源部件（widget）。这里设置为白板视图的视口（viewport），表明事件来源于白板的显示区域
        sceneEvent.setWidget(ui->whiteBoard->viewport());

        // 当前鼠标位置，将鼠标在视图（viewport）坐标系中的位置转换为场景坐标系中的位置
        // mouseEvent->pos()获取的是相对于视口的坐标，mapToScene()将其转换为场景中的坐标，这样绘图工具就知道在场景的哪个位置进行绘制
        sceneEvent.setScenePos(ui->whiteBoard->mapToScene(mouseEvent->pos()));

        // 设置鼠标在屏幕全局坐标系中的位置
        sceneEvent.setScreenPos(mouseEvent->globalPosition().toPoint());
        // 当前按钮，设置触发当前事件的鼠标按钮（如左键、右键、中键等）
        sceneEvent.setButton(mouseEvent->button());
        // 设置当前所有被按下的鼠标按钮状态
        sceneEvent.setButtons(mouseEvent->buttons());
        // 设置键盘修饰键的状态（如 Ctrl、Shift、Alt 等是否被按下）
        sceneEvent.setModifiers(mouseEvent->modifiers());

        switch (event->type()) {
        case QEvent::MouseButtonPress:
            m_drawingTool->mousePressEvent(&sceneEvent);
            break;
        case QEvent::MouseMove:
            m_drawingTool->mouseMoveEvent(&sceneEvent);
            break;
        case QEvent::MouseButtonRelease:
            m_drawingTool->mouseReleaseEvent(&sceneEvent);
            break;
        default:
            break;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

// 点击连接服务器就会触发这个槽函数
void Client::onConnectToServerClicked()
{
    m_connectionStatus->setStatus(LedIndicator::BlinkingYellow);
    ConnectDialog dialog(this);

    int result = dialog.exec(); // 先获取返回值
    std::cout << "exec() result: " << result << std::endl;
    std::cout << "QDialog::Accepted = " << QDialog::Accepted << std::endl;

    // 设置默认的端口和IP地址
    ip = this -> getLocalIp();
    // dialog.setIP(ip);
    // dialog.setPort(port);

    std::cout<<"ip = "<<dialog.getHost().toStdString()<<" port = "<<dialog.getPort()<<std::endl;

    if (result == QDialog::Accepted) {
        std::cout << "into if inner" << std::endl;

        QString url = dialog.getServerUrl();
        m_userName = dialog.getUserName();
        m_currentRoomId = dialog.getRoomId();

        ui -> room_id->setText("房间号: " + m_currentRoomId);
        ui -> client_id->setText("用户名: " + m_userName);

        std::cout << "URL: " << url.toStdString() << std::endl;
        std::cout << "user name: " << m_userName.toStdString() << std::endl;
        std::cout << "room ID: " << m_currentRoomId.toStdString() << std::endl;

        // 显示连接状态
        statusBar()->showMessage("正在连接到服务器: " + url);

        if (m_webSocketManager->connectToServer(url)) {
            // 连接成功后会触发&WebSocketManager::connected信号，然后执行槽函数 onConnectedToServer
            setOnlineMode(true);
        } else {
            statusBar()->showMessage("连接失败", 3000);
        }
    }
}

// 连接服务器同时加入到指定的房间号中
void Client::onConnectedToServer()
{
    statusBar()->showMessage("已连接到服务器");
    m_connectionStatus->setStatus(LedIndicator::BlinkingGreen);

    // 自动加入房间
    if (!m_currentRoomId.isEmpty()) {
        // 如果已经指定了房间号，直接调用joinRoom向服务端发送请求加入消息，并同步到其他的客户端
        m_webSocketManager->joinRoom(m_currentRoomId, m_userName);
    } else {
        // 如果连接成功之后没有指定房间号ID，就需要创建一个新房间号并提示用户输入，如果不输入房间号就随机的创建一个8位长度的房间号
        onJoinRoomClicked();
    }
}

void Client::onJoinRoomClicked()
{
    if (!m_webSocketManager->isConnected()) {
        statusBar()->showMessage("请先连接到服务器", 3000);
        return;
    }

    // 弹出房间选择对话框
    RoomDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // 如果没有房间号就创建一个
        QString roomId = dialog.getRoomId();
        if (roomId.isEmpty()) {
            // 创建新房间，如果没有指定房间号就创建随机的创建一个
            roomId = m_webSocketManager->createRoom(dialog.getRoomName());
        }
        ui -> room_id->setText("房间号: " + roomId);
        // 加入到指定的房间
        m_webSocketManager->joinRoom(roomId, m_userName);
    }
}

void Client::onDisconnectedFromServer()
{
    m_connectionStatus->setStatus(LedIndicator::BlinkingRed);
    statusBar()->showMessage("与服务器断开连接");
    setOnlineMode(false);
}

void Client::onConnectionError(const QString & error)
{
    Q_UNUSED(error);
    // ui->chat_frame->appendPlainText("❌ 连接错误: " + m_client.errorString());
    ui->connServer->setEnabled(true);
}
void Client::onDrawingOperationReceived(const DrawingOperation &operation)
{
    // 将网络接收到的绘图操作交给绘图工具处理
    if (m_drawingTool) {
        // std::cout<<"starting draw......"<<std::endl;
        m_drawingTool->processNetworkOperation(operation);
        // 如果接收到的消息是清除或者擦除对应的图形，那么需要更新网格，因为清除和擦除会影响到网格布局
        if(operation.opType == DrawingOperationType::DOT_Erase){
            updateGrid();
        }
    }
}

void Client::recvError(const QString&errorMessage){
    std::cout<<"server send error: "<<errorMessage.toStdString()<<std::endl;
}

// 颜色提取器
void Client::onColorBtnClicked()
{
    QColor color = QColorDialog::getColor(Qt::black, this, "选择颜色",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid() && m_drawingTool) {
        // 根据当前工具设置不同的颜色
        switch (m_drawingTool->getToolType()) {
        case DrawingTool::Pencil:
            m_drawingTool->setPenColor(color);
            m_drawingTool->setBrushColor(color); // 颜色填充
        case DrawingTool::Line:
            m_drawingTool->setPenColor(color);
            m_drawingTool->setBrushColor(color);
            break;
        case DrawingTool::Rectangle:
            m_drawingTool->setPenColor(color);
            m_drawingTool->setBrushColor(color);
        case DrawingTool::Ellipse:
            // 对于形状，可以同时设置边框和填充颜色
            m_drawingTool->setPenColor(color);
            m_drawingTool->setBrushColor(color.lighter(150)); // 稍亮的填充色
            break;
        case DrawingTool::Text:
            m_drawingTool->setTextColor(color);
            break;
        default:
            m_drawingTool->setPenColor(color);
            m_drawingTool->setBrushColor(color);
            break;
        }

        // 更新UI显示当前颜色
        updateColorDisplay(color);
    }
}

// 更新颜色显示
void Client::updateColorDisplay(const QColor &color)
{
    QPixmap colorIcon(24, 24);
    colorIcon.fill(color);
    ui->color_btn->setIcon(QIcon(colorIcon));

    // 可选：在状态栏显示当前颜色信息
    statusBar()->showMessage(QString("当前颜色: %1").arg(color.name()));
}

void Client::on_pen_btn_clicked()
{
    if(m_drawingTool->getIsEraser()){
        m_drawingTool->setPenColor(Qt::black);
        m_drawingTool->setBrushColor(Qt::black);
        m_drawingTool->setEraser(false);
    }
    m_drawingTool->setCurrentTool(DrawingTool::Pencil);
    updateColorDisplay(m_drawingTool->currentPenColor());
}


void Client::on_line_btn_clicked()
{
    if(m_drawingTool->getIsEraser()){
        m_drawingTool->setPenColor(Qt::black);
        m_drawingTool->setBrushColor(Qt::black);
        m_drawingTool->setEraser(false);
    }
    m_drawingTool->setCurrentTool(DrawingTool::Line);
    updateColorDisplay(m_drawingTool->currentPenColor());
}


void Client::on_elli_btn_clicked()
{
    if(m_drawingTool->getIsEraser()){
        m_drawingTool->setPenColor(Qt::black);
        m_drawingTool->setBrushColor(Qt::black);
        m_drawingTool->setEraser(false);
    }
    m_drawingTool->setCurrentTool(DrawingTool::Ellipse);
    updateColorDisplay(m_drawingTool->currentPenColor());
}


void Client::on_text_btn_clicked()
{
    if(m_drawingTool->getIsEraser()){
        m_drawingTool->setPenColor(Qt::black);
        m_drawingTool->setBrushColor(Qt::black);
        m_drawingTool->setEraser(false);
    }
    m_drawingTool->setCurrentTool(DrawingTool::Text);
    updateColorDisplay(m_drawingTool->currentPenColor());
}


void Client::on_rect_btn_clicked()
{
    if(m_drawingTool->getIsEraser()){
        m_drawingTool->setPenColor(Qt::black);
        m_drawingTool->setBrushColor(Qt::black);
        m_drawingTool->setEraser(false);
    }
    m_drawingTool->setCurrentTool(DrawingTool::Rectangle);
    updateColorDisplay(m_drawingTool->currentPenColor());
}


void Client::on_zoom_big_btn_clicked()
{
    ui->whiteBoard->scale(1.2, 1.2);
}


void Client::on_zoom_small_btn_clicked()
{
    ui->whiteBoard->scale(0.8, 0.8);
}

void Client::on_is_full_btn_clicked()
{
    is_full = !is_full;
    // std::cout<<is_full<<std::endl;
    m_drawingTool->setFull(is_full);
}

void Client::on_linew_spinBox_valueChanged(int arg1)
{
    m_drawingTool->setPenWidth(arg1);
}

void Client::on_eraser_btn_clicked()
{
    m_drawingTool->setCurrentTool(DrawingTool::Eraser);

    // 橡皮擦通常使用白色或透明色
    QColor eraserColor = Qt::white;
    m_drawingTool->setPenColor(eraserColor);
    m_drawingTool->setBrushColor(eraserColor);

    updateColorDisplay(eraserColor);
}

// 修改newFile方法
void Client::on_newFile()
{
    if (m_fileManager->newFile(ui->whiteBoard->scene())) {
        // 创建新文件后重新生成网格
        updateGrid();
    }
}

// 修改openFile方法
void Client::on_openFile()
{
    if (m_fileManager->openFile(ui->whiteBoard->scene())) {
        // 打开文件后重新生成网格
        updateGrid();
    }
}

void Client::on_saveFile()
{
    m_fileManager->saveFile(ui->whiteBoard->scene());
}

void Client::on_saveAsFile()
{
    m_fileManager->saveAsFile(ui->whiteBoard->scene());
}

void Client::on_exportImage()
{
    m_fileManager->exportToImage(ui->whiteBoard->scene());
}

// 清除场景
void Client::on_clearAction()
{
    // 确认对话框，避免误操作
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认清除",
                                  "确定要清除所有内容吗？此操作不可撤销。",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // 执行清楚白板操作
        m_drawingTool->clearScene();

        // 可选：重新添加网格背景
        QGraphicsScene *scene = ui->whiteBoard->scene();
        ui->whiteBoard->setScene(scene);

        // 清除后重新生成网格
        updateGrid();
    }
}

// 重做（恢复上次清除的内容）
void Client::on_redoAction()
{
    // 重做：把之前的内容恢复
    m_drawingTool->redo();

    // 更新修改状态
    if(!(m_fileManager -> getModified())){
        // 第一次更改的时候添加*，后面继续更改就不需要添加*了
        setWindowTitle(windowTitle() + " *");
    }
    m_fileManager->setModified(true);
}


// 切换网格显示
void Client::on_toggleGridAction()
{
    m_showGrid = !m_showGrid;
    updateGrid();

    // 更新菜单项文本
    if (m_showGrid) {
        ui->gridView->setText("隐藏网格");
    } else {
        ui->gridView->setText("显示网格");
    }
}

// 更新网格显示
void Client::updateGrid()
{
    QGraphicsScene *scene = ui->whiteBoard->scene();
    if (!scene) return;

    // 清除现有的网格项（通过标记识别）
    QList<QGraphicsItem*> items = scene->items();
    foreach (QGraphicsItem *item, items) {
        if (item->data(Qt::UserRole).toString() == "grid") {
            scene->removeItem(item);
            delete item;
        }
    }

    // 如果需要显示网格，则创建新的网格
    if (m_showGrid) {
        // 网格设置
        const int gridSize = 20;  // 网格大小
        const QColor gridColor = QColor(220, 220, 220, 150);  // 网格颜色（半透明）
        const int gridWidth = 1;  // 网格线宽

        QPen gridPen(gridColor, gridWidth);
        gridPen.setCosmetic(true);  //  cosmetic pen 不会随缩放而改变宽度

        // 获取场景矩形
        QRectF sceneRect = scene->sceneRect();
        if (sceneRect.isNull()) {
            // 如果场景矩形无效，设置一个默认的
            sceneRect = QRectF(0, 0, 800, 600);
            scene->setSceneRect(sceneRect);
        }

        // 创建水平网格线
        for (qreal y = sceneRect.top(); y <= sceneRect.bottom(); y += gridSize) {
            QGraphicsLineItem *line = new QGraphicsLineItem(sceneRect.left(), y, sceneRect.right(), y);
            line->setPen(gridPen);
            line->setZValue(-1000);  // 确保网格在最底层
            line->setData(Qt::UserRole, "grid");  // 标记为网格项
            line->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable); // 不可选择
            scene->addItem(line);
        }

        // 创建垂直网格线
        for (qreal x = sceneRect.left(); x <= sceneRect.right(); x += gridSize) {
            QGraphicsLineItem *line = new QGraphicsLineItem(x, sceneRect.top(), x, sceneRect.bottom());
            line->setPen(gridPen);
            line->setZValue(-1000);  // 确保网格在最底层
            line->setData(Qt::UserRole, "grid");  // 标记为网格项
            line->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable); // 不可选择
            scene->addItem(line);
        }

        // 可选：添加坐标轴或更明显的参考线
        // addReferenceLines(scene);
    }
}

void Client::onAboutTriggered()
{
    m_helpManager->showAboutDialog(this);
}

void Client::onHelpTriggered()
{
    m_helpManager->openHelpDocumentation();
}

void Client::onVersionTriggered()
{
    m_helpManager->showVersionDialog(this);
}

void Client::onCheckForUpdates()
{
    m_helpManager->checkForUpdates(false);
}

void Client::onUpdateAvailable(const QString &currentVersion, const QString &latestVersion)
{
    // 可以在这里添加更新提示，比如在状态栏显示
    statusBar()->showMessage(QString("发现新版本 %1 (当前: %2)").arg(latestVersion).arg(currentVersion), 5000);
}

void Client::onDrawingOperationCreated(const DrawingOperation &operation)
{
    if (m_webSocketManager && m_webSocketManager->isConnected()) {
        // 检查是否已加入房间
        if (m_webSocketManager->getCurrentRoomId().isEmpty()) {
            qWarning() << "尚未加入房间，无法发送绘图操作";
            return;
        }
        m_webSocketManager->sendDrawingOperation(operation);
    }
}

void Client::onClearSceneRequested()
{
    if (m_webSocketManager && m_webSocketManager->isConnected()) {
        m_webSocketManager->sendClearScene();
    }
}

void Client::onUndoRequested()
{
    if (m_webSocketManager && m_webSocketManager->isConnected()) {
        m_webSocketManager->sendUndoRequest();
    }
}

void Client::onRedoRequested()
{
    if (m_webSocketManager && m_webSocketManager->isConnected()) {
        m_webSocketManager->sendRedoRequest();
    }
}

void Client::onClearSceneReceived()
{
    m_drawingTool->clearScene();
}

void Client::onUndoRequestReceived()
{
    m_drawingTool->undo();
}

void Client::onRedoRequestReceived()
{
    m_drawingTool->redo();
}

void Client::setOnlineMode(bool online)
{
    m_isOnlineMode = online;
    m_drawingTool->setOnlineMode(online);
}

QString Client::getLocalIp()
{
    QString hostName = QHostInfo::localHostName();

    QStringList availableIps;
    QString preferredIp;

    // 获取所有网络接口
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach (const QNetworkInterface &interfac, interfaces) {
        // 跳过回环和未启用的接口
        if (interfac.flags().testFlag(QNetworkInterface::IsLoopBack) ||
            !interfac.flags().testFlag(QNetworkInterface::IsUp) ||
            !interfac.flags().testFlag(QNetworkInterface::IsRunning)) {
            continue;
        }

        // ui->chat_frame->appendPlainText("📡 网络接口: " + interface.humanReadableName());

        // 获取该接口的所有IP地址
        QList<QNetworkAddressEntry> entries = interfac.addressEntries();
        foreach (const QNetworkAddressEntry &entry, entries) {
            QHostAddress ip = entry.ip();
            // 判断当前IP协议是否为IPv4
            if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                QString ipStr = ip.toString();
                availableIps.append(ipStr);

                QString displayText;
                if (ip.isLoopback()) {
                    displayText = "➰ 回环: " + ipStr;
                } else if (ip.isInSubnet(QHostAddress("192.168.0.0"), 16)) {
                    displayText = "🏠 局域网: " + ipStr;
                    if (preferredIp.isEmpty()) preferredIp = ipStr;
                } else if (ip.isInSubnet(QHostAddress("10.0.0.0"), 8)) {
                    displayText = "🏠 局域网: " + ipStr;
                    if (preferredIp.isEmpty()) preferredIp = ipStr;
                } else if (ip.isInSubnet(QHostAddress("172.16.0.0"), 12)) {
                    displayText = "🏠 局域网: " + ipStr;
                    if (preferredIp.isEmpty()) preferredIp = ipStr;
                } else if (ip.isInSubnet(QHostAddress("169.254.0.0"), 16)) {
                    displayText = "🔗 链路本地: " + ipStr;
                } else {
                    displayText = "🌍 公网: " + ipStr;
                    if (preferredIp.isEmpty()) preferredIp = ipStr;
                }

                // 显示子网掩码
                displayText += " / " + entry.netmask().toString();
                // ui->chat_frame->appendPlainText("   " + displayText);
            }
        }
        // ui->chat_frame->appendPlainText("");
    }

    if (availableIps.isEmpty()) {
        return "127.0.0.1";
    }

    return preferredIp;
}

// 辅助函数：获取连接状态字符串
QString Client::getSocketStateString(QAbstractSocket::SocketState state)
{
    switch (state) {
        case QAbstractSocket::UnconnectedState: return "❌ 未连接";
        case QAbstractSocket::HostLookupState: return "🔍 正在查找主机...";
        case QAbstractSocket::ConnectingState: return "🔄 正在连接...";
        case QAbstractSocket::ConnectedState: return "✅ 已连接";
        case QAbstractSocket::ClosingState: return "⏹️ 正在关闭...";
        default: return "❓ 未知状态";
    }
}

void Client::disconnectClient(){
    m_webSocketManager->getSocket()->close();
    m_connectionStatus->setStatus(LedIndicator::BlinkingRed);
    statusBar()->showMessage("和服务端的连接断开");
}


void Client::onChatActionTriggered()
{
    if (m_chatDialog->isHidden()) {
        m_chatDialog->show();
        m_chatDialog->raise();
        m_chatDialog->activateWindow();
    } else {
        m_chatDialog->hide();
    }
}

void Client::onChatMessageSent(const QString &message)
{
    if (m_webSocketManager && m_webSocketManager->isConnected()) {
        // 使用新的sendChatMessage方法
        m_webSocketManager->sendChatMessage(message);

        // 在本地聊天框中显示自己的消息
        m_chatDialog->addMessage(m_userName, message);
    } else {
        QMessageBox::warning(this, "错误", "未连接到服务器，无法发送消息");
    }
}

void Client::onChatMessageReceived(const QString &userName, const QString &message)
{
    // 将消息添加到聊天框中
    m_chatDialog->addMessage(userName, message);

    // 如果聊天窗口隐藏，显示通知
    if (m_chatDialog->isHidden()) {
        // 可以添加系统通知或闪烁任务栏图标
        statusBar()->showMessage(QString("新消息来自 %1").arg(userName), 3000);
    }
}

void Client::setupUserListWidget()
{
    // 创建停靠窗口
    m_userListDock = new QDockWidget("用户列表", this);
    m_userListDock->setFeatures(QDockWidget::DockWidgetClosable | // 可关闭
                                QDockWidget::DockWidgetMovable | // 可移动
                                QDockWidget::DockWidgetFloatable); // 可悬浮
    // 左侧和右侧停靠区域
    m_userListDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    // 创建用户列表控件
    m_userListWidget = new QListWidget(m_userListDock);
    m_userListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置样式
    m_userListWidget->setStyleSheet(
        "QListWidget {"
        "    background-color: #f8f9fa;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 5px;"
        "}"
        "QListWidget::item {"
        "    padding: 8px;"
        "    border-bottom: 1px solid #e9ecef;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #e3f2fd;"
        "}"
        );

    // 设置停靠窗口的内容
    m_userListDock->setWidget(m_userListWidget);

    // 初始状态为隐藏
    m_userListDock->setVisible(false);

    // 添加到主窗口（但不显示）
    addDockWidget(Qt::RightDockWidgetArea, m_userListDock);
}

void Client::onUserInfoActionTriggered()
{
    toggleUserListVisibility();
}

void Client::toggleUserListVisibility()
{
    m_userListVisible = !m_userListVisible;
    m_userListDock->setVisible(m_userListVisible);

    // 更新菜单项文本
    if (m_userListVisible) {
        ui->user_info->setText("隐藏用户列表");
        // 如果用户列表为空，可以请求更新
        if (m_currentRoomUsers.isEmpty() && m_webSocketManager->isConnected()) {
            // 可以发送请求获取最新用户列表
            statusBar()->showMessage("正在加载用户列表...", 2000);
        }
    } else {
        ui->user_info->setText("显示用户列表");
    }
}

void Client::onClientListReceived(const QList<QJsonObject> &clients)
{
    // 清空当前用户列表
    m_currentRoomUsers.clear();
    m_userListWidget->clear();

    // 更新用户列表
    for (const QJsonObject &client : clients) {
        QString userId = client["userId"].toString();
        QString userName = client["userName"].toString();
        int role = client["role"].toInt();

        m_currentRoomUsers[userId] = client;

        // 添加到列表控件
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(userName);

        // 根据角色设置不同图标和样式
        switch (static_cast<UserRole>(role)) {
        case UR_Presenter:
            item->setIcon(QIcon(":/../images/presenter.png"));
            item->setForeground(Qt::blue);
            item->setToolTip("演示者 - 可以控制房间权限");
            break;
        case UR_Editor:
            item->setIcon(QIcon(":/../images/editor.png"));
            item->setToolTip("编辑者 - 可以编辑内容");
            break;
        case UR_Viewer:
            item->setIcon(QIcon(":/../images/viewer.png"));
            item->setForeground(Qt::gray);
            item->setToolTip("查看者 - 只能查看内容");
            break;
        }

        m_userListWidget->addItem(item);
    }

    // 更新状态栏
    statusBar()->showMessage(QString("房间内有 %1 个用户").arg(clients.size()), 3000);

    // 如果用户列表正在显示，确保它可见
    if (m_userListVisible && !m_userListDock->isVisible()) {
        m_userListDock->setVisible(true);
    }
}

void Client::onUserJoined(const QString &userId, const QString &userName, int role)
{
    // 添加新用户到列表
    QJsonObject client;
    client["userId"] = userId;
    client["userName"] = userName;
    client["role"] = role;

    m_currentRoomUsers[userId] = client;

    // 更新列表控件
    QListWidgetItem *item = new QListWidgetItem();
    item->setText(userName);

    // 设置角色图标和提示
    switch (static_cast<UserRole>(role)) {
        case UR_Presenter:
            item->setIcon(QIcon(":/../images/presenter.png"));
            item->setForeground(Qt::blue);
            item->setToolTip("演示者 - 可以控制房间权限");
            break;
        case UR_Editor:
            item->setIcon(QIcon(":/../images/editor.png"));
            item->setToolTip("编辑者 - 可以编辑内容");
            break;
        case UR_Viewer:
            item->setIcon(QIcon(":/../images/viewer.png"));
            item->setForeground(Qt::gray);
            item->setToolTip("查看者 - 只能查看内容");
            break;
    }

    m_userListWidget->addItem(item);

    // 显示通知
    statusBar()->showMessage(QString("%1 加入了房间").arg(userName), 3000);

    // 在聊天框中显示加入消息
    if (m_chatDialog) {
        m_chatDialog->addMessage("系统", QString("%1 加入了房间").arg(userName));
    }

    // 如果用户列表正在显示，确保它可见
    if (m_userListVisible && !m_userListDock->isVisible()) {
        m_userListDock->setVisible(true);
    }
}

// 客户端断开连接，那么需要将对应的客户端退出列表
void Client::onUserLeft(const QString &userId)
{
    if (m_currentRoomUsers.contains(userId)) {
        QString userName = m_currentRoomUsers[userId]["userName"].toString();

        // 从列表中移除
        m_currentRoomUsers.remove(userId);

        // 从列表控件中移除
        for (int i = 0; i < m_userListWidget->count(); ++i) {
            QListWidgetItem *item = m_userListWidget->item(i);
            if (item->text() == userName) {
                delete m_userListWidget->takeItem(i);
                break;
            }
        }

        // 显示通知
        statusBar()->showMessage(QString("%1 离开了房间").arg(userName), 3000);

        // 在聊天框中显示离开消息
        if (m_chatDialog) {
            m_chatDialog->addMessage("系统", QString("%1 离开了房间").arg(userName));
        }
    }
}


void Client::on_font_weight_valueChanged(int arg1)
{
    m_drawingTool->setFontWeight(arg1);
    std::cout<<"cur font weight = "<<arg1<<std::endl;
}

void Client::onUndoRequestedWithData(const DrawingOperation &operation)
{
    qDebug() << "处理带数据的撤销请求，操作类型:" << operation.opType;

    if (m_webSocketManager && m_webSocketManager->isConnected()) {
        try {
            // 创建网络消息
            NetworkMessage message;
            message.type = MT_UndoRequest;
            message.timestamp = QDateTime::currentSecsSinceEpoch();
            message.senderId = m_webSocketManager->getUserId();
            message.data = operation.toJson();

            // 发送到服务器
            m_webSocketManager->sendNetworkMessage(message);

            qDebug() << "已发送撤销请求到服务器";

        } catch (const std::exception& e) {
            qCritical() << "发送撤销请求失败:" << e.what();
            QMessageBox::warning(this, "错误", QString("发送撤销请求失败: %1").arg(e.what()));
        } catch (...) {
            qCritical() << "发送撤销请求发生未知错误";
            QMessageBox::warning(this, "错误", "发送撤销请求发生未知错误");
        }
    } else {
        qWarning() << "未连接到服务器，无法发送撤销请求";
        QMessageBox::warning(this, "错误", "未连接到服务器，无法发送撤销请求");
    }
}

void Client::onRedoRequestedWithData(const DrawingOperation &operation)
{
    qDebug() << "处理带数据的重做请求，操作类型:" << operation.opType;

    if (m_webSocketManager && m_webSocketManager->isConnected()) {
        try {
            // 创建网络消息
            NetworkMessage message;
            message.type = MT_RedoRequest;
            message.timestamp = QDateTime::currentSecsSinceEpoch();
            message.senderId = m_webSocketManager->getUserId();
            message.data = operation.toJson();

            // 发送到服务器
            m_webSocketManager->sendNetworkMessage(message);

            qDebug() << "已发送重做请求到服务器";

        } catch (const std::exception& e) {
            qCritical() << "发送重做请求失败:" << e.what();
            QMessageBox::warning(this, "错误", QString("发送重做请求失败: %1").arg(e.what()));
        } catch (...) {
            qCritical() << "发送重做请求发生未知错误";
            QMessageBox::warning(this, "错误", "发送重做请求发生未知错误");
        }
    } else {
        qWarning() << "未连接到服务器，无法发送重做请求";
        QMessageBox::warning(this, "错误", "未连接到服务器，无法发送重做请求");
    }
}
