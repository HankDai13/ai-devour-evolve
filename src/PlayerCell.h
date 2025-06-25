#ifndef PLAYERCELL_H
#define PLAYERCELL_H

#include <QGraphicsObject> // 使用QGraphicsObject以支持信号和槽
#include <QRectF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTimer>

class PlayerCell : public QGraphicsObject
{
    Q_OBJECT

public:
    PlayerCell(qreal x, qreal y, qreal radius);

    // QGraphicsItem的两个必须重写的纯虚函数
    QRectF boundingRect() const override; // 返回一个能完全包围我们item的矩形
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override; // 如何绘制自己

    // 获取半径的函数
    qreal radius() const { return m_radius; }
    
    // 设置半径的函数（为成长功能）
    void setRadius(qreal radius);
    
    // 获取玩家得分
    int score() const { return m_score; }

signals:
    void scoreChanged(int newScore); // 得分变化信号

private slots:
    void checkCollisions(); // 检查碰撞的槽函数

private:
    qreal m_radius; // 成员变量习惯以 m_ 开头
    QTimer *m_collisionTimer; // 碰撞检测定时器
    int m_score; // 玩家得分
    
    void eatFood(class FoodItem* food); // 吞噬食物的函数
};

#endif // PLAYERCELL_H
