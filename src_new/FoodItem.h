#ifndef FOODITEM_H
#define FOODITEM_H

#include <QGraphicsObject>
#include <QRectF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

class FoodItem : public QGraphicsObject
{
    Q_OBJECT

public:
    FoodItem(qreal x, qreal y, qreal radius = 8.0);

    // QGraphicsItem的必须重写的函数
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    // 获取食物的营养值（影响玩家成长幅度）
    qreal nutritionValue() const { return m_nutritionValue; }
    qreal radius() const { return m_radius; }

private:
    qreal m_radius;
    qreal m_nutritionValue;
    QColor m_color;
};

#endif // FOODITEM_H
