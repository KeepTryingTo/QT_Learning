#ifndef LEDINDICATOR_H
#define LEDINDICATOR_H

#include <QWidget>

#include <QWidget>
#include <QTimer>
#include <QPainter>

class LedIndicator : public QWidget {
    Q_OBJECT
public:
    enum Status { Off, Red, Green, Yellow, BlinkingGreen, BlinkingRed, BlinkingYellow };

    explicit LedIndicator(QWidget *parent = nullptr);

    void setStatus(Status status) {
        m_status = status;

        // 处理闪烁
        m_blinkTimer->stop();
        // 如果不使用BlinkingGreen, BlinkingRed, BlinkingYellow，那么定时器就不会启动，也就没闪烁的效果
        if (m_status == BlinkingGreen || m_status == BlinkingRed || m_status == BlinkingYellow) {
            m_blinking = true;
            m_blinkOn = true;
            m_blinkTimer->start(500); // 500ms间隔检查一次当前灯的状态
        } else {
            // 如果不是闪烁状态，设置闪烁标志为false。
            m_blinking = false;
        }

        // update() 函数的主要作用是请求重绘控件。它会安排一个绘制事件（paint event）
        update(); // 触发重绘
    }

protected:
    void paintEvent(QPaintEvent *) override {
        // 创建一个painter对象以及设置抗锯齿属性
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制背景，最后开始这个灯是暗的
        painter.setBrush(Qt::darkGray);
        // 在控件的整个矩形区域内绘制一个椭圆（圆形），作为指示灯的底座
        painter.drawEllipse(rect());

        // 根据状态设置颜色
        QColor color;
        bool drawLight = true;

        // 如果灯是关着的，后面就不要进行效果渲染了
        if (m_blinking && !m_blinkOn) {
            drawLight = false;
        } else {
            switch(m_status) {
            case Off:
                drawLight = false;
                break;
            case Red:
                color = Qt::red;
                break;
            case Green:
                color = Qt::green;
                break;
            case Yellow:
                color = Qt::yellow;
                break;
            case BlinkingGreen:
                color = Qt::green;
                break;
            case BlinkingRed:
                color = Qt::red;
                break;
            case BlinkingYellow:
                color = Qt::yellow;
                break;
            }
        }

        if (drawLight) {
            // 绘制发光效果 创建径向渐变，参数分别是：中心点(x,y)、半径、焦点(x,y)。
            // 这里之所以设置半径为8，是因为我们设置的区域大小为16 x 16
            QRadialGradient gradient(8, 8, 8, 5, 5);
            // 其中0表示渐变的起点为中心，在渐变中心设置较亮的颜色（亮度增加50%）
            gradient.setColorAt(0, color.lighter(150));
            // 其中1表示渐变的起点为边缘，在渐变边缘设置较暗的颜色（亮度减少100%）
            gradient.setColorAt(1, color.darker(200));
            // 设置为渐变的效果
            painter.setBrush(gradient);
            // 设置无边框（不绘制边缘线）
            painter.setPen(Qt::NoPen);
            // 在指定位置绘制椭圆（圆形），参数是：x坐标、y坐标、宽度、高度
            painter.drawEllipse(2, 2, 12, 12);
        }
    }

private slots:
    void toggleBlink() {
        // 切换灯的亮灭状态
        m_blinkOn = !m_blinkOn;
        update();
    }

private:
    Status m_status;
    // 表示正在“闪烁的”
    bool m_blinking;
    // 灯是否亮着
    bool m_blinkOn;
    // 定时器来定时检查当前灯的状态，用于闪烁效果
    QTimer *m_blinkTimer;
};

#endif // LEDINDICATOR_H
