#include "BaseBall.h"
#include <QGraphicsScene>
#include <QDebug>

BaseBall::BaseBall(int ballId, const QPointF& position, qreal score, const Border& border, BallType type, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_ballId(ballId)
    , m_score(score)
    , m_ballType(type)
    , m_border(border)
    , m_isRemoved(false)
    , m_velocity(0, 0)
    , m_acceleration(0, 0)
{
    setPos(position);
    updateRadius();
    checkBorder();
}

QRectF BaseBall::boundingRect() const
{
    return QRectF(-m_radius, -m_radius, 2 * m_radius, 2 * m_radius);
}

void BaseBall::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    // 获取球的颜色
    QColor ballColor = getBallColor();
    
    // 绘制球体
    painter->setBrush(QBrush(ballColor));
    painter->setPen(QPen(ballColor.darker(150), 2));
    painter->drawEllipse(QRectF(-m_radius, -m_radius, 2 * m_radius, 2 * m_radius));
    
    // 绘制高光效果
    QColor highlightColor = ballColor.lighter(180);
    highlightColor.setAlpha(100);
    painter->setBrush(QBrush(highlightColor));
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QRectF(-m_radius * 0.3, -m_radius * 0.3, m_radius * 0.6, m_radius * 0.6));
}

void BaseBall::setScore(qreal score)
{
    if (m_score != score) {
        m_score = score;
        updateRadius();
        emit scoreChanged(m_score);
    }
}

void BaseBall::move(const QVector2D& direction, qreal duration)
{
    // 基础移动实现 - 子类可以重写
    if (direction.length() > 0) {
        QVector2D normalizedDirection = direction.normalized();
        
        // 使用当前速度向量，而不是速度长度
        QVector2D displacement = m_velocity * duration;
        
        // 如果速度为0，使用方向和默认速度
        if (displacement.length() < 0.1) {
            displacement = normalizedDirection * 2.0 * duration; // 默认速度
        }
        
        QPointF oldPos = pos();
        QPointF newPos = pos() + QPointF(displacement.x(), displacement.y());
        setPos(newPos);
        
        checkBorder();
    }
}

bool BaseBall::canEat(BaseBall* other) const
{
    if (!other || other->isRemoved() || this->isRemoved()) {
        return false;
    }
    
    // 基本规则：需要比对方大一定比例才能吃掉
    qreal radiusRatio = m_radius / other->radius();
    qreal threshold = 1.10; // 默认阈值：大10%
    
    // 对于食物球，降低阈值使其更容易被吃掉
    if (other->ballType() == FOOD_BALL) {
        threshold = 1.05; // 只需要大5%即可吃掉食物
    }
    
    bool canEatResult = radiusRatio > threshold;
    
    qDebug() << "canEat check: eater radius=" << m_radius 
             << "target radius=" << other->radius() 
             << "ratio=" << radiusRatio 
             << "threshold=" << threshold
             << "result=" << canEatResult;
    
    return canEatResult;
}

void BaseBall::eat(BaseBall* other)
{
    if (canEat(other)) {
        // 增加分数
        qreal newScore = m_score + other->score();
        setScore(newScore);
        
        // 移除被吃掉的球
        other->remove();
        
        emit ballEaten(this, other);
    }
}

void BaseBall::remove()
{
    if (!m_isRemoved) {
        m_isRemoved = true;
        emit ballRemoved(this);
        
        // 从场景中移除
        if (scene()) {
            scene()->removeItem(this);
        }
    }
}

bool BaseBall::collidesWith(BaseBall* other) const
{
    if (!other || other == this) {
        return false;
    }
    
    qreal distance = distanceTo(other);
    return distance < (m_radius + other->radius());
}

qreal BaseBall::distanceTo(BaseBall* other) const
{
    if (!other) {
        return std::numeric_limits<qreal>::max();
    }
    
    QPointF thisPos = pos();
    QPointF otherPos = other->pos();
    
    qreal dx = thisPos.x() - otherPos.x();
    qreal dy = thisPos.y() - otherPos.y();
    
    return std::sqrt(dx * dx + dy * dy);
}

void BaseBall::checkBorder()
{
    QPointF currentPos = pos();
    QPointF newPos = currentPos;
    bool positionChanged = false;
    
    // 检查X边界
    if (currentPos.x() - m_radius < m_border.minx) {
        newPos.setX(m_border.minx + m_radius);
        positionChanged = true;
        m_velocity.setX(0); // 停止X方向的移动
    } else if (currentPos.x() + m_radius > m_border.maxx) {
        newPos.setX(m_border.maxx - m_radius);
        positionChanged = true;
        m_velocity.setX(0);
    }
    
    // 检查Y边界
    if (currentPos.y() - m_radius < m_border.miny) {
        newPos.setY(m_border.miny + m_radius);
        positionChanged = true;
        m_velocity.setY(0); // 停止Y方向的移动
    } else if (currentPos.y() + m_radius > m_border.maxy) {
        newPos.setY(m_border.maxy - m_radius);
        positionChanged = true;
        m_velocity.setY(0);
    }
    
    if (positionChanged) {
        setPos(newPos);
    }
}

qreal BaseBall::scoreToRadius(qreal score)
{
    // 根据GoBigger的公式：radius = sqrt(score / 100 * 0.042 + 0.15)
    return std::sqrt(score / 100.0 * 0.042 + 0.15) * 20.0; // 乘以20放大显示
}

qreal BaseBall::radiusToScore(qreal radius)
{
    // 逆向计算：score = ((radius / 20)^2 - 0.15) / 0.042 * 100
    qreal adjustedRadius = radius / 20.0;
    return (adjustedRadius * adjustedRadius - 0.15) / 0.042 * 100.0;
}

void BaseBall::updateRadius()
{
    qreal newRadius = scoreToRadius(m_score);
    if (std::abs(newRadius - m_radius) > 0.1) {
        m_radius = newRadius;
        update(); // 触发重绘
    }
}

void BaseBall::updatePhysics(qreal deltaTime)
{
    // 更新速度
    m_velocity += m_acceleration * deltaTime;
    
    // 应用移动
    if (m_velocity.length() > 0.01) {
        QPointF displacement(m_velocity.x() * deltaTime, m_velocity.y() * deltaTime);
        setPos(pos() + displacement);
        checkBorder();
    }
}
