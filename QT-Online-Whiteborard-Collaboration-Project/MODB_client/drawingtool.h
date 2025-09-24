// drawingtool.h
#ifndef DRAWINGTOOL_H
#define DRAWINGTOOL_H

#include <QObject>
#include <map>
#include <iostream>
#include <QVariantMap>
#include <QGraphicsScene>
#include <QInputDialog>
#include <QGraphicsSceneMouseEvent>

#include "networkprotocol.h"

class DrawingTool : public QObject
{
    Q_OBJECT
public:
    enum ToolType { Pencil, Line, Rectangle, Ellipse, Text, Select, Eraser };

    void setPen(const QPen &pen) { m_pen = pen; }
    void setBrush(const QBrush &brush) { m_brush = brush; }

    explicit DrawingTool(QGraphicsScene *scene, QObject *parent = nullptr);
    void setCurrentTool(ToolType tool);

    // 鼠标事件处理
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    void setFontWeight(int font_size){
        this -> font_weight = font_size;
    }
    void setPenColor(const QColor &color);
    void setPenWidth(int width);
    void setBrushColor(const QColor &color);
    void setTextColor(const QColor &color);
    void setFull(const bool is_full){this -> is_full = is_full;}
    void setEraser(const bool & is_eraser){this ->m_isErasing = is_eraser;}
    QColor currentPenColor() const;
    QColor currentBrushColor() const;
    QColor currentTextColor() const;

    ToolType getToolType(){
        return m_currentTool;
    }
    bool getIsEraser(){
        return this -> m_isErasing;
    }

    void clearScene();  // 清除场景
    void undo();  // 撤销
    void redo();  // 重做
    void saveState();  // 保存当前状态
    void restoreState(const QList<QGraphicsItem*>& state);

    // 添加网络相关方法
    void setOnlineMode(bool online);
    bool isOnlineMode() const;

    // 获取当前操作数据
    QVariantMap getCurrentOperationData(QGraphicsItem* finishedItem) const;
    DrawingOperationType getCurrentOperationType() const;

    // 添加处理网络操作的方法
    void processNetworkOperation(const DrawingOperation &operation);


public slots:
    // void undoErase();


signals:
    void toolChanged(DrawingTool::ToolType newTool);
    void contentModified();
    void sceneCleared(); // 场景清除信号

    // 添加网络操作信号
    void drawingOperationCreated(const DrawingOperation &operation);
    void clearSceneRequested();
    void undoRequested();
    void redoRequested();

    void undoRequestedWithData(const DrawingOperation &operation);
    void redoRequestedWithData(const DrawingOperation &operation);

private:
    QGraphicsScene *m_scene;
    // 绘制自由曲线、多边形、贝塞尔曲线等
    QGraphicsPathItem *m_currentPath;
    ToolType m_currentTool;
    QPen m_pen;
    QBrush m_brush;
    QColor m_textColor;

    // 是否填充
    bool is_full;
    // 绘画笔的宽度
    int pen_width;
    // 字体大小
    int font_weight;

    // 绘图临时变量
    QGraphicsItem *m_tempItem;
    // QPointF 表示一个浮点精度的二维坐标点
    QPointF m_startPoint;
    QPointF m_endPoint;
    bool m_isDrawing;

    // 各个工具的绘制方法
    void drawPencil(QGraphicsSceneMouseEvent *event);
    void drawLine(QGraphicsSceneMouseEvent *event);
    void drawRectangle(QGraphicsSceneMouseEvent *event);
    void drawEllipse(QGraphicsSceneMouseEvent *event);
    void drawText(QGraphicsSceneMouseEvent *event);

    QList<QGraphicsItem*> m_erasedItems; // 存储被擦除的项（用于可能的撤销）
    bool m_isErasing;                    // 标记是否正在擦除
    // 橡皮擦方法
    void eraseAtPosition(const QPointF &position);
    void drawEraser(QGraphicsSceneMouseEvent *event);

    // 用于保存撤销和重做结果
    QList<QList<QGraphicsItem*>> m_undoStack;  // 撤销栈
    QList<QList<QGraphicsItem*>> m_redoStack;  // 重做栈

    // 添加在线模式标志
    bool m_isOnlineMode;
    // 添加网络绘图相关的辅助方法
    QGraphicsPathItem *m_currentNetworkPath; // 用于跟踪网络接收的当前路径
    void drawNetworkPath(const QVariantMap &data);
    void drawNetworkLine(const QVariantMap &data);
    void drawNetworkRectangle(const QVariantMap &data);
    void drawNetworkEllipse(const QVariantMap &data);
    void addNetworkText(const QVariantMap &data);
    void performNetworkErase(const QVariantMap &data);
    bool isPathIntersecting(QGraphicsPathItem* pathItem, const QRectF& eraserArea);
    bool isLineIntersectingRect(const QPointF& p1, const QPointF& p2, const QRectF& rect);
    void processNetworkUndo(const QVariantMap &data);
    void processNetworkRedo(const QVariantMap &data);
    QList<QGraphicsItem*> getCurrentSceneState() const;

};

#endif // DRAWINGTOOL_H
