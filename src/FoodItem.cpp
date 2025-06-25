#include "FoodItem.h"
#include <QPainter>
#include <QPen>
#include <QRandomGenerator>

FoodItem::FoodItem(qreal x, qreal y, qreal radius) : m_radius(radius)
{
    // 设置位置
    setPos(x, y);
    
    // 根据大小设置营养值
    m_nutritionValue = m_radius * 0.5;
    
    // 随机选择食物颜色（不同颜色代表不同营养价值）
    QList<QColor> foodColors = {
        QColor(255, 100, 100), // 红色
        QColor(100, 255, 100), // 绿色
        QColor(255, 255, 100), // 黄色
        QColor(255, 150, 100), // 橙色
        QColor(150, 100, 255)  // 紫色
    };
    
    m_color = foodColors[QRandomGenerator::global()->bounded(foodColors.size())];
    
    // 设置一些标志
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    
    // 启用缓存以提高性能
    setCacheMode(QGraphicsItem::ItemCoordinateCache);
}

QRectF FoodItem::boundingRect() const
{
    return QRectF(-m_radius, -m_radius, m_radius * 2, m_radius * 2);
}

void FoodItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    // 设置抗锯齿
    painter->setRenderHint(QPainter::Antialiasing);
    
    // 设置填充颜色
    painter->setBrush(QBrush(m_color));
    
    // 设置边框
    QPen pen(m_color.darker(150), 1);
    painter->setPen(pen);
    
    // 绘制圆形食物
    painter->drawEllipse(QPointF(0, 0), m_radius, m_radius);
}
