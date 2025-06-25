#include "PlayerCell.h"
#include "FoodItem.h"
#include <QPainter>
#include <QPen>
#include <QGraphicsScene>
#include <QTimer>
#include <QDebug>

PlayerCell::PlayerCell(qreal x, qreal y, qreal radius) : m_radius(radius), m_score(0)
{
    // 设置初始位置
    setPos(x, y);
    
    // 设置一些标志以优化渲染
    setFlag(QGraphicsItem::ItemIsMovable, false); // 禁用默认的拖拽移动，我们自己控制
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setCacheMode(QGraphicsItem::ItemCoordinateCache); // 使用Item坐标缓存，更适合移动物体
    
    // 设置碰撞检测定时器
    m_collisionTimer = new QTimer(this);
    connect(m_collisionTimer, &QTimer::timeout, this, &PlayerCell::checkCollisions);
    m_collisionTimer->start(16); // 60FPS检测碰撞
}

// 告诉Qt，我们的绘图区域是多大
QRectF PlayerCell::boundingRect() const
{
    // 返回以中心为原点，边长为直径的正方形区域
    return QRectF(-m_radius, -m_radius, m_radius * 2, m_radius * 2);
}

// 具体的绘制逻辑
void PlayerCell::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option) // 消除编译器未使用参数的警告
    Q_UNUSED(widget)
    
    // 设置抗锯齿，让圆形更平滑
    painter->setRenderHint(QPainter::Antialiasing);
    
    // 设置画刷为蓝色
    painter->setBrush(QBrush(QColor(0, 120, 255))); // 漂亮的蓝色
    
    // 设置边框
    QPen pen(QColor(0, 80, 200), 2); // 深蓝色边框，宽度2像素
    painter->setPen(pen);
    
    // 以(0,0)为中心，画一个半径为m_radius的圆
    painter->drawEllipse(QPointF(0, 0), m_radius, m_radius);
}

void PlayerCell::setRadius(qreal radius)
{
    if (radius != m_radius) {
        prepareGeometryChange(); // 通知Qt几何形状即将改变
        m_radius = radius;
        update(); // 请求重绘
    }
}

void PlayerCell::checkCollisions()
{
    // 获取所有与玩家碰撞的物品
    QList<QGraphicsItem*> collidingItems = this->collidingItems();
    
    for (QGraphicsItem* item : collidingItems) {
        // 检查是否是食物
        FoodItem* food = qgraphicsitem_cast<FoodItem*>(item);
        if (food) {
            eatFood(food);
        }
    }
}

void PlayerCell::eatFood(FoodItem* food)
{
    if (!food || !scene()) return;
    
    // 增加得分
    int points = static_cast<int>(food->nutritionValue() * 10);
    m_score += points;
    
    // 增加半径（成长效果）
    qreal growthAmount = food->nutritionValue() * 0.3;
    setRadius(m_radius + growthAmount);
    
    // 发出得分变化信号
    emit scoreChanged(m_score);
    
    // 从场景中移除食物
    scene()->removeItem(food);
    delete food;
    
    // 在控制台输出得分（调试用）
    qDebug() << "Score:" << m_score << "Radius:" << m_radius;
}
