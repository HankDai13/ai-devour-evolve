#include "CloneBallData.h"
#include "SporeBallData.h"
#include "../../GoBiggerConfig.h"
#include <cmath>
#include <QRandomGenerator>

CloneBallData::CloneBallData(int ballId, const QPointF& position, const Border& border, int teamId, int playerId)
    : BaseBallData(ballId, CLONE_BALL, position, border)
    , m_teamId(teamId)
    , m_playerId(playerId)
    , m_moveDirection(0, 0)
    , m_framesSinceLastSplit(GoBiggerConfig::MERGE_DELAY + 1) // 初始状态允许合并
{
    setScore(GoBiggerConfig::CELL_INIT_SCORE);
}

void CloneBallData::move(const QVector2D& direction, qreal duration)
{
    if (direction.length() > 0) {
        // 使用GoBigger的动态速度计算
        float speed = GoBiggerConfig::calculateDynamicSpeed(m_radius);
        QVector2D normalizedDir = direction.normalized();
        QVector2D displacement = normalizedDir * speed * duration;
        
        m_position += QPointF(displacement.x(), displacement.y());
        m_moveDirection = normalizedDir;
        
        // 边界约束
        constrainToBorder();
    }
}

void CloneBallData::applyGoBiggerMovement(const QVector2D& playerInput, const QVector2D& centerForce)
{
    if (playerInput.length() == 0 && centerForce.length() == 0) {
        return;
    }
    
    // GoBigger原版的移动计算
    float deltaTime = 1.0f / 60.0f; // 60 FPS
    
    // 计算加速度
    QVector2D totalAcceleration = playerInput * GoBiggerConfig::calculateDynamicAcceleration(m_radius) + centerForce;
    
    // 更新速度
    m_velocity += totalAcceleration * deltaTime;
    
    // 应用阻尼
    m_velocity *= 0.95f;
    
    // 限制最大速度
    float maxSpeed = GoBiggerConfig::calculateDynamicSpeed(m_radius);
    if (m_velocity.length() > maxSpeed) {
        m_velocity = m_velocity.normalized() * maxSpeed;
    }
    
    // 更新位置
    QPointF displacement(m_velocity.x() * deltaTime, m_velocity.y() * deltaTime);
    m_position += displacement;
    
    // 边界约束
    constrainToBorder();
}

bool CloneBallData::canSplit() const
{
    return GoBiggerConfig::canSplit(m_score, 1); // TODO: 传入实际的分身数量
}

QVector<CloneBallData*> CloneBallData::performSplit(const QVector2D& direction)
{
    QVector<CloneBallData*> newBalls;
    
    if (!canSplit()) {
        return newBalls;
    }
    
    // 分裂成两个球
    float newScore = m_score / 2.0f;
    
    // 创建新球
    CloneBallData* newBall = new CloneBallData(0, m_position, m_border, m_teamId, m_playerId); // ballId由管理器分配
    newBall->setScore(newScore);
    
    // 设置分裂位置和速度
    QVector2D splitDir = direction.length() > 0 ? direction.normalized() : QVector2D(1, 0);
    float splitDistance = (m_radius + newBall->radius()) * 1.2f;
    
    // 原球位置调整
    m_position -= QPointF(splitDir.x() * splitDistance * 0.5f, splitDir.y() * splitDistance * 0.5f);
    
    // 新球位置
    QPointF newPos = m_position + QPointF(splitDir.x() * splitDistance, splitDir.y() * splitDistance);
    newBall->setPosition(newPos);
    
    // 设置分裂后的速度（继承原球速度）
    QVector2D splitVelocity = splitDir * GoBiggerConfig::SPLIT_BOOST_SPEED;
    newBall->setVelocity(m_velocity + splitVelocity);
    this->setVelocity(m_velocity - splitVelocity * 0.5f);
    
    // 更新分数
    this->setScore(newScore);
    
    // 重置冷却
    this->resetSplitCooldown();
    newBall->resetSplitCooldown();
    
    newBalls.append(newBall);
    return newBalls;
}

QVector<CloneBallData*> CloneBallData::performThornsSplit(const QVector2D& direction, int totalPlayerBalls)
{
    QVector<CloneBallData*> newBalls;
    
    // GoBigger荆棘分裂机制
    int maxNewBalls = qMin(GoBiggerConfig::THORNS_SPLIT_MAX_COUNT, 
                          GoBiggerConfig::MAX_SPLIT_COUNT - totalPlayerBalls);
    
    if (maxNewBalls <= 0) return newBalls;
    
    // 分配分数
    float totalScore = m_score;
    float newBallScore = qMin(static_cast<float>(GoBiggerConfig::THORNS_SPLIT_MAX_SCORE), 
                             totalScore / (maxNewBalls + 1));
    
    // 创建新球
    for (int i = 0; i < maxNewBalls; ++i) {
        CloneBallData* newBall = new CloneBallData(0, m_position, m_border, m_teamId, m_playerId);
        newBall->setScore(newBallScore);
        
        // 随机分布位置
        float angle = 2.0f * M_PI * i / maxNewBalls;
        QVector2D offset(cos(angle), sin(angle));
        float distance = (m_radius + newBall->radius()) * 1.5f;
        
        QPointF newPos = m_position + QPointF(offset.x() * distance, offset.y() * distance);
        newBall->setPosition(newPos);
        
        // 设置速度
        newBall->setVelocity(m_velocity + offset * 200.0f);
        newBall->resetSplitCooldown();
        
        newBalls.append(newBall);
    }
    
    // 更新原球分数
    this->setScore(totalScore - newBallScore * maxNewBalls);
    this->resetSplitCooldown();
    
    return newBalls;
}

bool CloneBallData::canEject() const
{
    return GoBiggerConfig::canEject(m_score);
}

SporeBallData* CloneBallData::ejectSpore(const QVector2D& direction)
{
    if (!canEject()) {
        return nullptr;
    }
    
    // 计算孢子初始位置
    QVector2D ejectDir = direction.length() > 0 ? direction.normalized() : QVector2D(1, 0);
    float sporeDistance = m_radius + 20.0f; // 孢子生成距离
    QPointF sporePos = m_position + QPointF(ejectDir.x() * sporeDistance, ejectDir.y() * sporeDistance);
    
    // 创建孢子
    SporeBallData* spore = new SporeBallData(0, sporePos, m_border, m_teamId, m_playerId);
    spore->setScore(GoBiggerConfig::EJECT_SCORE);
    
    // 设置孢子速度
    QVector2D sporeVelocity = ejectDir * GoBiggerConfig::EJECT_SPEED;
    spore->setVelocity(sporeVelocity);
    
    // 减少分数
    float newScore = m_score * (1.0f - GoBiggerConfig::EJECT_COST_RATIO);
    this->setScore(newScore);
    
    return spore;
}

bool CloneBallData::canMergeWith(const CloneBallData* other) const
{
    if (!other || other == this || other->isRemoved()) return false;
    if (other->teamId() != m_teamId || other->playerId() != m_playerId) return false;
    
    // 检查冷却时间
    if (m_framesSinceLastSplit < GoBiggerConfig::MERGE_DELAY || 
        other->framesSinceLastSplit() < GoBiggerConfig::MERGE_DELAY) {
        return false;
    }
    
    // 检查距离
    float distance = distanceTo(other);
    float mergeDistance = (m_radius + other->radius()) * GoBiggerConfig::RECOMBINE_RADIUS;
    
    return distance <= mergeDistance;
}

void CloneBallData::mergeWith(CloneBallData* other)
{
    if (!canMergeWith(other)) return;
    
    // 合并分数
    float combinedScore = m_score + other->score();
    
    // 计算质心位置
    float totalMass = m_score + other->score();
    QPointF newPos = (m_position * m_score + other->position() * other->score()) / totalMass;
    
    // 合并速度
    QVector2D combinedVelocity = (m_velocity * m_score + other->velocity() * other->score()) / totalMass;
    
    // 更新当前球
    this->setScore(combinedScore);
    this->setPosition(newPos);
    this->setVelocity(combinedVelocity);
    this->resetSplitCooldown();
    
    // 标记其他球为已移除
    other->markAsRemoved();
}

bool CloneBallData::shouldRigidCollide(const CloneBallData* other) const
{
    if (!other || other == this) return false;
    if (other->teamId() != m_teamId || other->playerId() != m_playerId) return false;
    
    // 在冷却期内进行刚体碰撞
    return (m_framesSinceLastSplit < GoBiggerConfig::MERGE_DELAY || 
            other->framesSinceLastSplit() < GoBiggerConfig::MERGE_DELAY);
}

void CloneBallData::rigidCollision(CloneBallData* other)
{
    if (!other || other == this) return;
    
    // 计算分离向量
    QPointF posA = this->position();
    QPointF posB = other->position();
    QVector2D separation(posB.x() - posA.x(), posB.y() - posA.y());
    
    float distance = separation.length();
    if (distance == 0) return;
    
    float totalRadius = this->radius() + other->radius();
    float overlap = totalRadius - distance;
    
    if (overlap > 0) {
        // 标准化分离向量
        QVector2D normal = separation.normalized();
        
        // 根据质量分配推开距离
        float massA = this->score();
        float massB = other->score();
        float totalMass = massA + massB;
        
        float pushA = overlap * (massB / totalMass);
        float pushB = overlap * (massA / totalMass);
        
        // 应用位置调整
        this->setPosition(posA - QPointF(normal.x() * pushA, normal.y() * pushA));
        other->setPosition(posB + QPointF(normal.x() * pushB, normal.y() * pushB));
        
        // 边界约束
        this->constrainToBorder();
        other->constrainToBorder();
    }
}

bool CloneBallData::canEat(const BaseBallData* other) const
{
    if (!other || other->isRemoved() || this->isRemoved()) return false;
    return GoBiggerConfig::canEat(m_score, other->score());
}

void CloneBallData::eat(BaseBallData* other)
{
    if (!canEat(other)) return;
    
    float gainedScore = other->score();
    this->setScore(m_score + gainedScore);
    other->markAsRemoved();
}

void CloneBallData::updatePhysics(qreal deltaTime)
{
    // 更新冷却计数器
    m_framesSinceLastSplit++;
    
    // 应用速度衰减
    m_velocity *= 0.98f;
    
    // 更新位置
    m_position += QPointF(m_velocity.x() * deltaTime, m_velocity.y() * deltaTime);
    
    // 边界约束
    constrainToBorder();
}
