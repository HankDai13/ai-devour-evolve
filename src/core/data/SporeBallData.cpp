#include "SporeBallData.h"
#include "../../GoBiggerConfig.h"

SporeBallData::SporeBallData(int ballId, const QPointF& position, const Border& border, int teamId, int playerId)
    : BaseBallData(ballId, SPORE_BALL, position, border)
    , m_teamId(teamId)
    , m_playerId(playerId)
    , m_lifespan(GoBiggerConfig::SPORE_LIFESPAN)
    , m_framesSinceCreation(0)
{
    setScore(GoBiggerConfig::EJECT_SCORE);
}

bool SporeBallData::canEat(const BaseBallData* other) const
{
    // 孢子不能吃任何东西
    Q_UNUSED(other)
    return false;
}

void SporeBallData::eat(BaseBallData* other)
{
    // 孢子不能吃任何东西
    Q_UNUSED(other)
}

void SporeBallData::updatePhysics(qreal deltaTime)
{
    m_framesSinceCreation++;
    
    // 更新生命周期
    updateLifespan();
    
    // 应用速度衰减
    if (m_framesSinceCreation <= GoBiggerConfig::EJECT_VEL_ZERO_FRAME) {
        // 在指定帧数内逐渐减速到0
        float decayFactor = 1.0f - (float)m_framesSinceCreation / GoBiggerConfig::EJECT_VEL_ZERO_FRAME;
        m_velocity *= decayFactor;
    } else {
        m_velocity = QVector2D(0, 0);
    }
    
    // 更新位置
    m_position += QPointF(m_velocity.x() * deltaTime, m_velocity.y() * deltaTime);
    
    // 边界约束
    constrainToBorder();
}

void SporeBallData::updateLifespan()
{
    m_lifespan--;
    if (m_lifespan <= 0) {
        markAsRemoved();
    }
}
