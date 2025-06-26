#include "BaseBall.h"
#include <QDebug>
#include <QGraphicsScene>
#include <cmath>

BaseBall::BaseBall(int ballId, const QPointF& position, float mass, const Border& border, BallType type, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_ballId(ballId)
    , m_mass(mass)
    , m_ballType(type)
    , m_border(border)
    , m_isRemoved(false)
    , m_velocity(0, 0)
    , m_acceleration(0, 0)
{
    updateRadius();
    setPos(position);
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
}

QRectF BaseBall::boundingRect() const
{
    qreal margin = 2.0;
    return QRectF(-m_radius - margin, -m_radius - margin, 
                  2 * (m_radius + margin), 2 * (m_radius + margin));
}

void BaseBall::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(QBrush(getBallColor()));
    painter->setPen(QPen(getBallColor().darker(120), 2));
    painter->drawEllipse(QRectF(-m_radius, -m_radius, 2 * m_radius, 2 * m_radius));
}

void BaseBall::setMass(float mass)
{
    if (mass != m_mass) {
        m_mass = std::max(1.0f, mass); // 确保质量至少为1
        updateRadius();
        emit massChanged(m_mass);
        
        qDebug() << "Ball" << m_ballId << "mass updated to" << m_mass 
                 << "radius:" << m_radius;
    }
}

void BaseBall::updateRadius()
{
    m_radius = std::sqrt(m_mass / M_PI) * GoBiggerConfig::MASS_TO_RADIUS_RATIO;
}

void BaseBall::move(const QVector2D& direction, qreal duration)
{
    // 基础移动实现 - 更丝滑的移动
    if (direction.length() > 0.01) {
        // 使用GoBigger标准速度计算，但更平滑
        float maxSpeed = GoBiggerConfig::BASE_SPEED / std::sqrt(m_mass / GoBiggerConfig::CELL_MIN_MASS);
        
        // 更平滑的加速度控制
        QVector2D targetVelocity = direction.normalized() * maxSpeed;
        QVector2D accel = (targetVelocity - m_velocity) * GoBiggerConfig::ACCELERATION_FACTOR; // 降低加速度
        
        setAcceleration(accel);
    } else {
        // 没有输入时应用更平滑的阻力
        setAcceleration(-m_velocity * 1.5f);
    }
    
    updatePhysics(duration);
}

void BaseBall::updatePhysics(qreal deltaTime)
{
    // 更新速度
    m_velocity += m_acceleration * deltaTime;
    
    // 应用更平滑的阻力
    m_velocity *= 0.99f; // 更轻的阻力，让移动更丝滑
    
    // 应用移动
    if (m_velocity.length() > 0.01) {
        QPointF displacement(m_velocity.x() * deltaTime, m_velocity.y() * deltaTime);
        setPos(pos() + displacement);
        checkBorder();
    }
}

bool BaseBall::canEat(BaseBall* other) const
{
    if (!other || other->isRemoved() || this->isRemoved()) {
        return false;
    }
    
    // 使用GoBigger标准吞噬比例
    bool canEatResult = m_mass >= other->mass() * GoBiggerConfig::EAT_RATIO;
    
    qDebug() << "canEat check: eater mass=" << m_mass 
             << "target mass=" << other->mass() 
             << "ratio=" << (m_mass / other->mass())
             << "threshold=" << GoBiggerConfig::EAT_RATIO
             << "result=" << canEatResult;
    
    return canEatResult;
}

void BaseBall::eat(BaseBall* other)
{
    if (!canEat(other)) {
        return;
    }
    
    float gainedMass = other->mass();
    float newMass = m_mass + gainedMass;
    
    qDebug() << "Ball" << m_ballId << "eating ball" << other->ballId()
             << "gained mass:" << gainedMass 
             << "new total mass:" << newMass;
    
    setMass(newMass);
    other->remove();
    
    emit ballEaten(this, other);
}

void BaseBall::remove()
{
    if (!m_isRemoved) {
        m_isRemoved = true;
        emit ballRemoved(this);
        qDebug() << "Ball" << m_ballId << "removed";
    }
}

bool BaseBall::collidesWith(BaseBall* other) const
{
    if (!other || other == this || other->isRemoved() || this->isRemoved()) {
        return false;
    }
    
    qreal distance = distanceTo(other);
    qreal collisionDistance = (m_radius + other->radius()) * GoBiggerConfig::EAT_DISTANCE_RATIO;
    
    return distance <= collisionDistance;
}

qreal BaseBall::distanceTo(BaseBall* other) const
{
    if (!other) return 0.0;
    
    QPointF diff = pos() - other->pos();
    return std::sqrt(diff.x() * diff.x() + diff.y() * diff.y());
}

void BaseBall::checkBorder()
{
    QPointF currentPos = pos();
    bool posChanged = false;
    
    if (currentPos.x() - m_radius < m_border.minx) {
        currentPos.setX(m_border.minx + m_radius);
        m_velocity.setX(0);
        posChanged = true;
    } else if (currentPos.x() + m_radius > m_border.maxx) {
        currentPos.setX(m_border.maxx - m_radius);
        m_velocity.setX(0);
        posChanged = true;
    }
    
    if (currentPos.y() - m_radius < m_border.miny) {
        currentPos.setY(m_border.miny + m_radius);
        m_velocity.setY(0);
        posChanged = true;
    } else if (currentPos.y() + m_radius > m_border.maxy) {
        currentPos.setY(m_border.maxy - m_radius);
        m_velocity.setY(0);
        posChanged = true;
    }
    
    if (posChanged) {
        setPos(currentPos);
    }
}
