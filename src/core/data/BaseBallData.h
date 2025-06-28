#ifndef BASEBALLDATA_H
#define BASEBALLDATA_H

#include <QPointF>
#include <QVector2D>

// 纯数据类：不依赖任何GUI组件
class BaseBallData
{
public:
    enum BallType {
        CLONE_BALL,
        FOOD_BALL,
        SPORE_BALL,
        THORNS_BALL
    };

    struct Border {
        qreal minx, maxx, miny, maxy;
        
        Border(qreal minX = -1000, qreal maxX = 1000, qreal minY = -1000, qreal maxY = 1000)
            : minx(minX), maxx(maxX), miny(minY), maxy(maxY) {}
    };

    BaseBallData(int ballId, BallType type, const QPointF& position, const Border& border);
    virtual ~BaseBallData() = default;

    // 基础属性访问
    int ballId() const { return m_ballId; }
    void setBallId(int id) { m_ballId = id; } // 添加setter
    BallType ballType() const { return m_ballType; }
    QPointF position() const { return m_position; }
    qreal radius() const { return m_radius; }
    float score() const { return m_score; }
    bool isRemoved() const { return m_isRemoved; }
    
    // 设置方法
    void setPosition(const QPointF& pos) { m_position = pos; }
    void setRadius(qreal radius) { m_radius = radius; }
    void setScore(float score);
    void markAsRemoved() { m_isRemoved = true; }
    
    // 物理相关
    QVector2D velocity() const { return m_velocity; }
    void setVelocity(const QVector2D& vel) { m_velocity = vel; }
    
    // 碰撞检测
    virtual bool collidesWith(const BaseBallData* other) const;
    qreal distanceTo(const BaseBallData* other) const;
    
    // 边界检查
    void constrainToBorder();
    bool isWithinBorder() const;
    
    // 纯虚函数 - 子类必须实现
    virtual bool canEat(const BaseBallData* other) const = 0;
    virtual void eat(BaseBallData* other) = 0;
    virtual void updatePhysics(qreal deltaTime) = 0;

protected:
    int m_ballId;
    BallType m_ballType;
    QPointF m_position;
    qreal m_radius;
    float m_score;
    bool m_isRemoved;
    QVector2D m_velocity;
    Border m_border;
    
    // 帮助函数
    void updateRadiusFromScore();
};

#endif // BASEBALLDATA_H
