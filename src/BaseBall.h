#ifndef BASEBALL_H
#define BASEBALL_H

#include <QGraphicsObject>
#include <QPointF>
#include <QVector2D>
#include <QRectF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTimer>
#include <cmath>
#include "GoBiggerConfig.h"

// 边界结构
struct Border {
    qreal minx, maxx, miny, maxy;
    
    Border(qreal minx = -2000, qreal maxx = 2000, qreal miny = -2000, qreal maxy = 2000)
        : minx(minx), maxx(maxx), miny(miny), maxy(maxy) {}
    
    bool contains(const QPointF& point) const {
        return point.x() >= minx && point.x() <= maxx && 
               point.y() >= miny && point.y() <= maxy;
    }
};

// 球的基础类
class BaseBall : public QGraphicsObject
{
    Q_OBJECT

public:
    enum BallType {
        CLONE_BALL,    // 玩家球（可控制）
        FOOD_BALL,     // 食物球
        SPORE_BALL,    // 孢子球
        THORNS_BALL    // 荆棘球
    };

    BaseBall(int ballId, const QPointF& position, float mass, const Border& border, BallType type, QGraphicsItem* parent = nullptr);
    
    virtual ~BaseBall() = default;

    // QGraphicsItem必须实现的函数
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    // 基础属性访问
    int ballId() const { return m_ballId; }
    float mass() const { return m_mass; }
    float radius() const { return m_radius; }
    BallType ballType() const { return m_ballType; }
    const Border& border() const { return m_border; }
    bool isRemoved() const { return m_isRemoved; }
    
    // 位置和速度
    QVector2D velocity() const { return m_velocity; }
    QVector2D acceleration() const { return m_acceleration; }
    
    // 设置属性
    void setMass(float mass);
    void setVelocity(const QVector2D& velocity) { m_velocity = velocity; }
    void setAcceleration(const QVector2D& acceleration) { m_acceleration = acceleration; }
    
    // 核心功能
    virtual void move(const QVector2D& direction, qreal duration);
    virtual bool canEat(BaseBall* other) const;
    virtual void eat(BaseBall* other);
    virtual void remove();
    
    // 碰撞检测
    bool collidesWith(BaseBall* other) const;
    qreal distanceTo(BaseBall* other) const;
    
    // 边界检查
    void checkBorder();
    
    // 物理更新
    virtual void updatePhysics(qreal deltaTime);

signals:
    void ballRemoved(BaseBall* ball);
    void ballEaten(BaseBall* eater, BaseBall* eaten);
    void massChanged(float newMass);

protected:
    // 属性
    int m_ballId;
    float m_mass;
    float m_radius;
    BallType m_ballType;
    Border m_border;
    bool m_isRemoved;
    
    // 物理属性
    QVector2D m_velocity;
    QVector2D m_acceleration;
    
    // 更新半径
    // 更新半径
    void updateRadius();
    
    // 获取球的颜色（由子类实现）
    virtual QColor getBallColor() const = 0;
};

#endif // BASEBALL_H
