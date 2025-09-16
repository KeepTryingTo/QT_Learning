#ifndef REMOTE_SCREEN_WIDGET_H
#define REMOTE_SCREEN_WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QPixmap>
#include <QThread>
#include <QMutex>
#include <QMouseEvent>

class RemoteScreenWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit RemoteScreenWidget(QWidget *parent = nullptr);
    ~RemoteScreenWidget();

public slots:
    void updateFrame(const QPixmap &frame);
    void setVisible(bool visible) override;


signals:
    void visibilityChanged(bool visible);
    // 鼠标移动事件，操作远程桌面
    void remoteMouseEvent(QPoint position, Qt::MouseButton button, QString action);

    // 添加键盘事件信号
    void remoteKeyEvent(int key, Qt::KeyboardModifiers modifiers, const QString &text, bool isPress);

protected:
    // 重写虚函数
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    // 添加键盘事件处理
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    QPixmap m_currentFrame;
    QMutex m_frameMutex;
};

// 独立的渲染画面线程类
class RenderThread : public QThread
{
    Q_OBJECT
public:
    explicit RenderThread(RemoteScreenWidget *widget, QObject *parent = nullptr);
    void run() override;

signals:
    void renderRequest();

private:
    RemoteScreenWidget *m_widget;
};


#endif
