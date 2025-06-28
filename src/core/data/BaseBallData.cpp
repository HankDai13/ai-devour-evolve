#include "BaseBallData.h"
#include "../../GoBiggerConfig.h"
#include <cmath>

BaseBallData::BaseBallData(int ballId, BallType type, const QPointF& position, const Border& border)
    : m_ballId(ballId)
    , m_ballType(type)
    , m_position(position)
    , m_radius(GoBiggerConfig::CELL_MIN_RADIUS)
    , m_score(GoBiggerConfig::CELL_MIN_SCORE)
    , m_isRemoved(false)
    , m_velocity(0, 0)
    , m_border(border)
{
    updateRadiusFromScore();
}

void BaseBallData::setScore(float score)
{
    m_score = qMax(static_cast<float>(GoBiggerConfig::CELL_MIN_SCORE), score);
    updateRadiusFromScore();
}

bool BaseBallData::collidesWith(const BaseBallData* other) const
{
    if (!other || other->isRemoved() || this->isRemoved()) {
        return false;
    }
    
    qreal distance = distanceTo(other);
    qreal minDistance = this->radius() + other->radius();
    
    return distance <= minDistance;
}

qreal BaseBallData::distanceTo(const BaseBallData* other) const
{
    if (!other) return std::numeric_limits<qreal>::max();
    
    QPointF otherPos = other->position();
    qreal dx = m_position.x() - otherPos.x();
    qreal dy = m_position.y() - otherPos.y();
    
    return std::sqrt(dx * dx + dy * dy);
}

void BaseBallData::constrainToBorder()
{
    qreal x = m_position.x();
    qreal y = m_position.y();
    
    // 考虑半径，确保球体完全在边界内
    x = qBound(m_border.minx + m_radius, x, m_border.maxx - m_radius);
    y = qBound(m_border.miny + m_radius, y, m_border.maxy - m_radius);
    
    m_position = QPointF(x, y);
}

bool BaseBallData::isWithinBorder() const
{
    return (m_position.x() - m_radius >= m_border.minx &&
            m_position.x() + m_radius <= m_border.maxx &&
            m_position.y() - m_radius >= m_border.miny &&
            m_position.y() + m_radius <= m_border.maxy);
}

void BaseBallData::updateRadiusFromScore()
{
    m_radius = GoBiggerConfig::scoreToRadius(m_score);
    m_radius = qBound(GoBiggerConfig::CELL_MIN_RADIUS, m_radius, GoBiggerConfig::CELL_MAX_RADIUS);
}
