#include "ThornsBallData.h"
#include "SporeBallData.h"
#include "../../GoBiggerConfig.h"
#include <QRandomGenerator>

ThornsBallData::ThornsBallData(int ballId, const QPointF& position, const Border& border)
    : BaseBallData(ballId, THORNS_BALL, position, border)
    , m_isMoving(false)
    , m_moveFramesLeft(0)
    , m_colorIndex(QRandomGenerator::global()->bounded(4))
{
    // 随机分数范围
    int score = QRandomGenerator::global()->bounded(
        GoBiggerConfig::THORNS_MAX_SCORE - GoBiggerConfig::THORNS_MIN_SCORE + 1) + 
        GoBiggerConfig::THORNS_MIN_SCORE;
    setScore(score);
}

void ThornsBallData::eatSpore(SporeBallData* spore)
{
    if (!spore || spore->isRemoved()) return;
    
    // 荆棘球吃孢子，获得移动能力
    QVector2D sporeDirection = spore->velocity().normalized();
    if (sporeDirection.length() > 0) {
        m_velocity = sporeDirection * GoBiggerConfig::THORNS_SPORE_SPEED;
        m_isMoving = true;
        m_moveFramesLeft = GoBiggerConfig::THORNS_SPORE_DECAY_FRAMES;
    }
    
    // 移除孢子
    spore->markAsRemoved();
}

bool ThornsBallData::canEat(const BaseBallData* other) const
{
    // 荆棘球只能吃孢子
    return other && !other->isRemoved() && other->ballType() == SPORE_BALL;
}

void ThornsBallData::eat(BaseBallData* other)
{
    if (other && other->ballType() == SPORE_BALL) {
        eatSpore(static_cast<SporeBallData*>(other));
    }
}

void ThornsBallData::updatePhysics(qreal deltaTime)
{
    updateMovement();
    
    // 更新位置
    if (m_isMoving && m_velocity.length() > 0) {
        m_position += QPointF(m_velocity.x() * deltaTime, m_velocity.y() * deltaTime);
        constrainToBorder();
    }
}

void ThornsBallData::updateMovement()
{
    if (m_isMoving) {
        m_moveFramesLeft--;
        
        if (m_moveFramesLeft <= 0) {
            m_isMoving = false;
            m_velocity = QVector2D(0, 0);
        } else {
            // 速度逐渐衰减
            float decayFactor = (float)m_moveFramesLeft / GoBiggerConfig::THORNS_SPORE_DECAY_FRAMES;
            m_velocity *= decayFactor;
        }
    }
}
