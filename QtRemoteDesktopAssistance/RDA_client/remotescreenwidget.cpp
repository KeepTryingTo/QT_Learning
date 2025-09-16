#include "remotescreenwidget.h"
#include <QPainter>

RemoteScreenWidget::RemoteScreenWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setAutoFillBackground(false);

    setFocusPolicy(Qt::StrongFocus); // 确保设置了焦点策略
    qDebug() << "Focus policy:" << focusPolicy(); // 检查焦点策略是否生效
}

RemoteScreenWidget::~RemoteScreenWidget()
{
    makeCurrent();
    doneCurrent();
}

void RemoteScreenWidget::initializeGL()
{
    // 现在这个函数是已声明的
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void RemoteScreenWidget::paintGL()
{
    // 注意这里加锁
    QMutexLocker locker(&m_frameMutex);

    // 清除背景
    glClear(GL_COLOR_BUFFER_BIT);

    if (m_currentFrame.isNull()) {
        return;
    }

    // 用于绘制的对象
    QPainter painter(this);
    painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

    // 计算缩放比例（计算当前画布的高宽和当前帧高宽的比例）
    qreal ratio = qMin(static_cast<qreal>(width()) / m_currentFrame.width(),
                       static_cast<qreal>(height()) / m_currentFrame.height());

    // 计算目标尺寸和位置
    QSize targetSize = m_currentFrame.size() * ratio;
    QRect targetRect(QPoint(0, 0), targetSize);
    // 移动到布局的中心
    targetRect.moveCenter(rect().center());

    // 绘制图像
    painter.drawPixmap(targetRect, m_currentFrame);

    // 可选：绘制边框
    painter.setPen(Qt::gray);
    painter.drawRect(targetRect.adjusted(-1, -1, 1, 1));
}

// 缩放当前布局大小
void RemoteScreenWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void RemoteScreenWidget::updateFrame(const QPixmap &frame)
{
    // 加锁并重绘制当前布局
    QMutexLocker locker(&m_frameMutex);
    m_currentFrame = frame;
    update();
}

void RemoteScreenWidget::setVisible(bool visible)
{
    // 设置当前局部可见还是不可见，并发送一个信号出去
    QOpenGLWidget::setVisible(visible);
    // 必须加入这句代码，必须让远程桌面的屏幕显示的获得焦距之后才能保证后面正常的输入
    if (visible) {
        setFocus(); // 显式获取焦点
        qDebug() << "Widget visible, has focus:" << hasFocus(); // 检查是否成功获取焦点
    }
    emit visibilityChanged(visible);
}

// RenderThread 实现
RenderThread::RenderThread(RemoteScreenWidget *widget, QObject *parent)
    : QThread(parent), m_widget(widget)
{
}

void RenderThread::run()
{
    // ​​安全地检查线程是否被请求中断
    while (!isInterruptionRequested()) {
        // 触发渲染
        emit renderRequest();
        msleep(16); // ~60 FPS
    }
}

// 鼠标按下事件
void RemoteScreenWidget::mousePressEvent(QMouseEvent *event)
{
    // 判断服务端是否正常把屏幕帧给传输过来了
    if (m_currentFrame.isNull()) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }

    // 计算点击位置相对于原始图像的比例
    qreal ratio = qMin(static_cast<qreal>(width()) / m_currentFrame.width(),
                       static_cast<qreal>(height()) / m_currentFrame.height());
    QSize targetSize = m_currentFrame.size() * ratio;
    QRect targetRect(QPoint(0, 0), targetSize);
    targetRect.moveCenter(rect().center());

    // 判断鼠标点击的位置是否在屏幕帧范围之内
    if (targetRect.contains(event->pos())) {
        // 相对的坐标比例计算
        QPointF relativePos(
            (event->pos().x() - targetRect.x()) / static_cast<qreal>(targetRect.width()),
            (event->pos().y() - targetRect.y()) / static_cast<qreal>(targetRect.height())
            );
        // 相对坐标 * 实际屏幕帧的高宽得到实际的坐标
        QPoint actualPos(
            static_cast<int>(relativePos.x() * m_currentFrame.width()),
            static_cast<int>(relativePos.y() * m_currentFrame.height())
            );

        emit remoteMouseEvent(actualPos, event->button(), "press");
    }

    QOpenGLWidget::mousePressEvent(event);
}
// 鼠标移动
void RemoteScreenWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_currentFrame.isNull()) {
        QOpenGLWidget::mouseMoveEvent(event);
        return;
    }

    // 计算移动位置相对于原始图像的比例
    qreal ratio = qMin(static_cast<qreal>(width()) / m_currentFrame.width(),
                       static_cast<qreal>(height()) / m_currentFrame.height());
    QSize targetSize = m_currentFrame.size() * ratio;
    QRect targetRect(QPoint(0, 0), targetSize);
    targetRect.moveCenter(rect().center());

    if (targetRect.contains(event->pos())) {
        QPointF relativePos(
            (event->pos().x() - targetRect.x()) / static_cast<qreal>(targetRect.width()),
            (event->pos().y() - targetRect.y()) / static_cast<qreal>(targetRect.height())
            );

        QPoint actualPos(
            static_cast<int>(relativePos.x() * m_currentFrame.width()),
            static_cast<int>(relativePos.y() * m_currentFrame.height())
            );
        // 把实际的坐标通过信号发送响应出去
        emit remoteMouseEvent(actualPos, Qt::NoButton, "move");
    }

    QOpenGLWidget::mouseMoveEvent(event);
}

// 鼠标释放
void RemoteScreenWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_currentFrame.isNull()) {
        QOpenGLWidget::mouseReleaseEvent(event);
        return;
    }

    // 计算释放位置相对于原始图像的比例
    qreal ratio = qMin(static_cast<qreal>(width()) / m_currentFrame.width(),
                       static_cast<qreal>(height()) / m_currentFrame.height());
    QSize targetSize = m_currentFrame.size() * ratio;
    QRect targetRect(QPoint(0, 0), targetSize);
    targetRect.moveCenter(rect().center());

    if (targetRect.contains(event->pos())) {
        QPointF relativePos(
            (event->pos().x() - targetRect.x()) / static_cast<qreal>(targetRect.width()),
            (event->pos().y() - targetRect.y()) / static_cast<qreal>(targetRect.height())
            );

        QPoint actualPos(
            static_cast<int>(relativePos.x() * m_currentFrame.width()),
            static_cast<int>(relativePos.y() * m_currentFrame.height())
            );

        emit remoteMouseEvent(actualPos, event->button(), "release");
    }

    QOpenGLWidget::mouseReleaseEvent(event);
}

// 键盘输入事件处理
void RemoteScreenWidget::keyPressEvent(QKeyEvent *event)
{
    // 聚焦
    qDebug() << "Key press event received" << event->key(); // 调试输出
    if (hasFocus()) {
        // 表示按键产生的实际字符内容
        QString text = event->text();
        if (text.isEmpty() && event->key() >= Qt::Key_Space && event->key() <= Qt::Key_ydiaeresis) {
            text = QChar(event->key());
        }
        qDebug() << "Sending key event:" << event->key(); // 调试输出
        // modifiers: 表示事件发生时按下的修饰键组合状态
        emit remoteKeyEvent(event->key(), event->modifiers(), text, true);
    }

    QOpenGLWidget::keyPressEvent(event);
}

void RemoteScreenWidget::keyReleaseEvent(QKeyEvent *event)
{
    // 聚焦
    if (hasFocus()) {
        emit remoteKeyEvent(event->key(), event->modifiers(), event->text(), false);
    }

    QOpenGLWidget::keyReleaseEvent(event);
}

void RemoteScreenWidget::focusInEvent(QFocusEvent *event)
{
    // 设置聚焦策略
    /*
        enum FocusPolicy {
            NoFocus       = 0,    // 完全不接受焦点
            TabFocus      = 0x1,  // 仅通过Tab键获取焦点
            ClickFocus    = 0x2,  // 仅通过鼠标点击获取焦点
            StrongFocus   = TabFocus | ClickFocus | WheelFocus,  // 支持所有方式
            WheelFocus    = 0x4   // 通过鼠标滚轮滚动获取焦点
        };
    */
    setFocusPolicy(Qt::StrongFocus);
    // 调用基类的焦点事件处理函数，确保焦点切换时执行Qt默认行为
    QOpenGLWidget::focusInEvent(event);
    update();
}

void RemoteScreenWidget::focusOutEvent(QFocusEvent *event)
{
    // 可以通过RemoteKeyboardController的m_pressedKeys来跟踪
    QOpenGLWidget::focusOutEvent(event);
    update();
}
