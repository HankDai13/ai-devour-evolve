#include "CloneBall.h"
#include "SporeBall.h"
#include "FoodBall.h"
#include <QRandomGenerator>
#include <QGraphicsScene>
#include <QDebug>
#include <cmath>

CloneBall::CloneBall(int ballId, const QPointF& position, const Border& border, int teamId, int playerId, 
                     const Config& config, QGraphicsItem* parent)
    : BaseBall(ballId, position, GoBiggerConfig::CELL_MIN_MASS, border, CLONE_BALL, parent) // 使用标准初始质量
    , m_config(config)
    , m_teamId(teamId)
    , m_playerId(playerId)
    , m_moveDirection(0, 0)
    , m_splitVelocity(0, 0)
    , m_splitVelocityPiece(0, 0)
    , m_splitFrame(0)
    , m_frameSinceLastSplit(0)
    , m_fromSplit(false)
    , m_fromThorns(false)
    , m_splitParent(nullptr)
    , m_movementTimer(nullptr)
    , m_decayTimer(nullptr)
{
    initializeTimers();
    updateDirection();
}

CloneBall::~CloneBall()
{
    if (m_movementTimer) {
        m_movementTimer->stop();
        delete m_movementTimer;
    }
    if (m_decayTimer) {
        m_decayTimer->stop();
        delete m_decayTimer;
    }
}

void CloneBall::initializeTimers()
{
    // 移动更新定时器
    m_movementTimer = new QTimer(this);
    connect(m_movementTimer, &QTimer::timeout, this, &CloneBall::updateMovement);
    m_movementTimer->start(16); // ~60 FPS
    
    // 分数衰减定时器
    m_decayTimer = new QTimer(this);
    connect(m_decayTimer, &QTimer::timeout, this, &CloneBall::updateScoreDecay);
    m_decayTimer->start(100); // 每100ms检查一次衰减
}

bool CloneBall::canSplit() const
{
    // 使用GoBigger标准分裂质量
    return m_mass >= GoBiggerConfig::SPLIT_MIN_MASS && m_frameSinceLastSplit >= m_config.recombineFrame;
}

bool CloneBall::canEject() const
{
    // 使用GoBigger标准质量计算是否可以喷射孢子  
    return m_mass >= GoBiggerConfig::EJECT_MASS * 2; // 至少要有足够质量才能喷射
}

void CloneBall::setMoveDirection(const QVector2D& direction)
{
    m_moveDirection = direction.normalized();
    updateDirection();
    
    // 如果有分裂组，则统一控制整个组
    propagateMovementToGroup(direction);
    
    // 立即应用移动
    if (direction.length() > 0.01) {
        move(direction, 0.016); // 以16ms为时间步长移动
    }
}

QVector<CloneBall*> CloneBall::performSplit(const QVector2D& direction)
{
    QVector<CloneBall*> newBalls;
    
    if (!canSplit()) {
        return newBalls;
    }
    
    // 计算分裂后的质量 - 使用GoBigger标准
    float splitMass = m_mass / 2.0f;
    
    // 创建新的球
    CloneBall* newBall = new CloneBall(
        m_ballId + 1000, // 临时ID策略
        pos() + QPointF(radius() * 1.5, 0), // 稍微偏移位置
        m_border,
        m_teamId,
        m_playerId,
        m_config
    );
    
    // 设置质量
    setMass(splitMass);
    newBall->setMass(splitMass);
    
    // 应用分裂冲刺速度
    QVector2D splitDir = direction.normalized();
    QVector2D splitVelocity = splitDir * GoBiggerConfig::SPLIT_BOOST_SPEED;
    
    setVelocity(splitVelocity);
    newBall->setVelocity(-splitVelocity); // 反方向
    
    // 重置分裂计时器
    m_frameSinceLastSplit = 0;
    newBall->m_frameSinceLastSplit = 0;
    
    // 标记分裂关系 - 用于合并和统一控制
    newBall->setSplitParent(this);
    m_splitChildren.append(newBall);
    
    newBalls.append(newBall);
    
    // 添加到场景
    if (scene()) {
        scene()->addItem(newBall);
    }
    
    emit splitPerformed(this, newBalls);
    
    return newBalls;
}

SporeBall* CloneBall::ejectSpore(const QVector2D& direction)
{
    if (!canEject()) {
        return nullptr;
    }
    
    // 使用GoBigger标准孢子质量和消耗
    float sporeMass = GoBiggerConfig::EJECT_MASS;
    float massLoss = m_mass * GoBiggerConfig::EJECT_COST_RATIO;
    massLoss = std::max(massLoss, sporeMass); // 至少消耗孢子质量
    
    QVector2D sporeDirection = direction.normalized();
    
    // 减少自己的质量
    setMass(m_mass - massLoss);
    
    // 创建孢子球
    QPointF sporePos = pos() + QPointF(sporeDirection.x() * (radius() + 15), 
                                       sporeDirection.y() * (radius() + 15));
    
    SporeBall* spore = new SporeBall(
        m_ballId + 2000, // 临时ID策略
        sporePos,
        m_border,
        m_teamId,
        m_playerId
    );
    
    spore->setMass(sporeMass);
    spore->setInitialVelocity(sporeDirection * GoBiggerConfig::EJECT_SPEED); // 使用标准速度
    
    // 添加到场景
    if (scene()) {
        scene()->addItem(spore);
    }
    
    emit sporeEjected(this, spore);
    
    return spore;
}

void CloneBall::move(const QVector2D& direction, qreal duration)
{
    if (direction.length() > 0.01) {
        // 计算最大速度（基于球的质量） - 使用GoBigger标准，但更平滑
        qreal maxSpeed = GoBiggerConfig::BASE_SPEED / std::sqrt(m_mass / GoBiggerConfig::CELL_MIN_MASS);
        
        // 更直接的速度控制，减少延迟感
        QVector2D targetVelocity = direction.normalized() * maxSpeed;
        
        // 加速度控制，但响应更快
        QVector2D accel = (targetVelocity - velocity()) * GoBiggerConfig::ACCELERATION_FACTOR;
        setAcceleration(accel);
        
        // 立即更新速度，但使用插值让变化更平滑
        QVector2D currentVel = velocity();
        QVector2D newVel = currentVel + accel * duration;
        
        // 限制最大速度
        if (newVel.length() > maxSpeed) {
            newVel = newVel.normalized() * maxSpeed;
        }
        
        // 使用更平滑的插值让速度变化更自然
        QVector2D velocityDiff = newVel - currentVel;
        
        // 限制每帧最大速度变化量，防止抽搐
        float maxChange = maxSpeed * 0.15f; // 每帧最多变化15%的最大速度
        if (velocityDiff.length() > maxChange) {
            velocityDiff = velocityDiff.normalized() * maxChange;
        }
        
        setVelocity(currentVel + velocityDiff);
    } else {
        // 如果没有输入方向，应用更平滑的阻力
        QVector2D currentVel = velocity();
        if (currentVel.length() > 0.1) {
            setVelocity(currentVel * 0.90); // 更平滑的减速
        } else {
            setVelocity(QVector2D(0, 0)); // 完全停止
        }
    }
    
    // 应用分裂速度
    if (m_splitVelocity.length() > 0.1) {
        QVector2D totalVel = velocity() + m_splitVelocity;
        setVelocity(totalVel);
        
        // 衰减分裂速度
        m_splitVelocity -= m_splitVelocityPiece;
        if (m_splitVelocity.length() < 0.1) {
            m_splitVelocity = QVector2D(0, 0);
        }
    }
    
    // 调用基类物理更新
    updatePhysics(duration);
}

bool CloneBall::canEat(BaseBall* other) const
{
    if (!other || other->isRemoved() || this->isRemoved()) {
        return false;
    }
    
    // 不能吃同队的球
    if (other->ballType() == CLONE_BALL) {
        CloneBall* otherClone = static_cast<CloneBall*>(other);
        if (otherClone->teamId() == m_teamId) {
            return false;
        }
    }
    
    // 基本大小规则
    return BaseBall::canEat(other);
}

void CloneBall::eat(BaseBall* other)
{
    if (canEat(other)) {
        BaseBall::eat(other);
        qDebug() << "CloneBall" << ballId() << "ate" << other->ballId() << "gaining mass:" << other->mass();
    }
}

QColor CloneBall::getBallColor() const
{
    QColor teamColor = getTeamColor(m_teamId);
    
    // 玩家球使用更饱和、更鲜艳的颜色
    teamColor.setHsv(teamColor.hue(), 
                     qMin(255, teamColor.saturation() + 50), 
                     qMin(255, teamColor.value() + 30));
    
    return teamColor;
}

void CloneBall::updatePhysics(qreal deltaTime)
{
    BaseBall::updatePhysics(deltaTime);
    
    // 增加分裂计时器
    m_frameSinceLastSplit++;
    m_splitFrame++;
}

void CloneBall::updateMovement()
{
    const qreal deltaTime = 0.016; // 16ms ≈ 60 FPS
    
    // 如果有移动方向，持续应用移动
    if (m_moveDirection.length() > 0.01) {
        move(m_moveDirection, deltaTime);
    }
    
    updatePhysics(deltaTime);
    
    // 更新分裂状态
    if (m_splitFrame > 0) {
        m_splitFrame++;
    }
    
    // 检查合并条件
    checkForMerge();
}

void CloneBall::updateScoreDecay()
{
    applyScoreDecay();
}

void CloneBall::updateDirection()
{
    // 更新内部方向向量（用于渲染等）
    if (m_moveDirection.length() > 0) {
        // 可以在这里添加方向相关的逻辑
    }
}

qreal CloneBall::calculateSplitVelocityFromSplit(qreal radius) const
{
    // 根据GoBigger的逻辑计算分裂速度
    return std::max(0.0, 20.0 - radius * 0.5);
}

qreal CloneBall::calculateSplitVelocityFromThorns(qreal radius) const
{
    // 碰到荆棘时的分裂速度更大
    return std::max(0.0, 30.0 - radius * 0.3);
}

void CloneBall::applySplitVelocity(const QVector2D& direction, bool fromThorns)
{
    qreal splitVelMagnitude = fromThorns ? 
        calculateSplitVelocityFromThorns(radius()) :
        calculateSplitVelocityFromSplit(radius());
    
    m_splitVelocity = direction.normalized() * splitVelMagnitude;
    m_splitVelocityPiece = m_splitVelocity / m_config.splitVelZeroFrame;
    
    m_fromSplit = !fromThorns;
    m_fromThorns = fromThorns;
}

void CloneBall::applySplitVelocityEnhanced(const QVector2D& direction, qreal velocity, bool fromThorns)
{
    // 增强的分裂速度应用，支持非线性衰减
    m_splitVelocity = direction.normalized() * velocity;
    
    // 非线性衰减：使用平方根衰减而不是线性衰减
    qreal decayFrames = m_config.splitVelZeroFrame;
    m_splitVelocityPiece = m_splitVelocity / (decayFrames * 0.7); // 稍微减慢衰减
    
    m_fromSplit = !fromThorns;
    m_fromThorns = fromThorns;
}

void CloneBall::propagateMovementToGroup(const QVector2D& direction)
{
    // 统一控制分裂出的所有球
    for (CloneBall* child : m_splitChildren) {
        if (child && !child->isRemoved()) {
            child->m_moveDirection = direction.normalized();
            child->updateDirection();
        }
    }
    
    // 如果自己是子球，也要控制兄弟球
    if (m_splitParent && !m_splitParent->isRemoved()) {
        QVector<CloneBall*> siblings = m_splitParent->getSplitChildren();
        for (CloneBall* sibling : siblings) {
            if (sibling && sibling != this && !sibling->isRemoved()) {
                sibling->m_moveDirection = direction.normalized();
                sibling->updateDirection();
            }
        }
    }
}

void CloneBall::applyScoreDecay()
{
    // 使用GoBigger标准衰减
    if (m_mass > GoBiggerConfig::DECAY_START_MASS) {
        float decay = m_mass * GoBiggerConfig::DECAY_RATE;
        setMass(std::max((float)GoBiggerConfig::CELL_MIN_MASS, m_mass - decay));
    }
}

QColor CloneBall::getTeamColor(int teamId) const
{
    // 为不同团队提供不同颜色
    QList<QColor> teamColors = {
        QColor(0, 120, 255),   // 蓝色 - 团队0
        QColor(255, 60, 60),   // 红色 - 团队1
        QColor(60, 255, 60),   // 绿色 - 团队2
        QColor(255, 200, 0),   // 黄色 - 团队3
        QColor(255, 0, 255),   // 品红 - 团队4
        QColor(0, 255, 255),   // 青色 - 团队5
        QColor(255, 128, 0),   // 橙色 - 团队6
        QColor(128, 0, 255),   // 紫色 - 团队7
    };
    
    int colorIndex = teamId % teamColors.size();
    return teamColors[colorIndex];
}

void CloneBall::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    // 获取球的颜色
    QColor ballColor = getBallColor();
    
    // 绘制外圈（玩家球特有的边框）
    QPen borderPen(ballColor.darker(120), 3);
    painter->setPen(borderPen);
    painter->setBrush(QBrush(ballColor));
    painter->drawEllipse(QRectF(-radius(), -radius(), 2 * radius(), 2 * radius()));
    
    // 绘制内圈渐变
    QRadialGradient gradient(0, 0, radius());
    gradient.setColorAt(0, ballColor.lighter(150));
    gradient.setColorAt(0.7, ballColor);
    gradient.setColorAt(1, ballColor.darker(120));
    
    painter->setBrush(QBrush(gradient));
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QRectF(-radius() * 0.9, -radius() * 0.9, 
                                2 * radius() * 0.9, 2 * radius() * 0.9));
    
    // 绘制高光效果
    QColor highlightColor = ballColor.lighter(200);
    highlightColor.setAlpha(120);
    painter->setBrush(QBrush(highlightColor));
    painter->drawEllipse(QRectF(-radius() * 0.3, -radius() * 0.3, 
                                radius() * 0.6, radius() * 0.6));
    
    // 绘制团队标识（小圆点）
    QColor teamDot = ballColor.darker(50);
    painter->setBrush(QBrush(teamDot));
    painter->drawEllipse(QRectF(-radius() * 0.1, -radius() * 0.1, 
                                radius() * 0.2, radius() * 0.2));
}

// ============ 合并机制实现 ============

bool CloneBall::canMergeWith(CloneBall* other) const
{
    if (!other || other == this || other->isRemoved() || this->isRemoved()) {
        return false;
    }
    
    // 必须是同队同玩家
    if (other->teamId() != m_teamId || other->playerId() != m_playerId) {
        return false;
    }
    
    // 必须超过合并延迟时间(使用帧计算，假设60FPS)
    int mergeDelayFrames = GoBiggerConfig::MERGE_DELAY * 60; // 20秒 * 60帧
    if (m_frameSinceLastSplit < mergeDelayFrames || other->frameSinceLastSplit() < mergeDelayFrames) {
        return false;
    }
    
    // 距离必须足够近
    qreal distance = distanceTo(other);
    qreal mergeDistance = (m_radius + other->radius()) * GoBiggerConfig::RECOMBINE_RADIUS;
    
    return distance <= mergeDistance;
}

void CloneBall::mergeWith(CloneBall* other)
{
    if (!canMergeWith(other)) {
        return;
    }
    
    // 合并质量
    float combinedMass = m_mass + other->mass();
    setMass(combinedMass);
    
    // 合并速度(加权平均)
    QVector2D combinedVelocity = (m_velocity * m_mass + other->velocity() * other->mass()) / combinedMass;
    setVelocity(combinedVelocity);
    
    // 重置分裂计时器
    m_frameSinceLastSplit = 0;
    
    // 移除被合并的球
    other->remove();
    
    // 从分裂关系中移除
    m_splitChildren.removeOne(other);
    if (other->getSplitParent() == this) {
        other->setSplitParent(nullptr);
    }
    
    qDebug() << "Ball" << m_ballId << "merged with ball" << other->ballId() 
             << "new mass:" << combinedMass;
}

void CloneBall::checkForMerge()
{
    // 检查与兄弟球的合并
    for (CloneBall* child : m_splitChildren) {
        if (canMergeWith(child)) {
            mergeWith(child);
            break; // 一次只合并一个
        }
    }
    
    // 如果自己是子球，检查与父球和其他兄弟球的合并
    if (m_splitParent && canMergeWith(m_splitParent)) {
        m_splitParent->mergeWith(this);
        return; // 自己被合并了，直接返回
    }
    
    if (m_splitParent) {
        QVector<CloneBall*> siblings = m_splitParent->getSplitChildren();
        for (CloneBall* sibling : siblings) {
            if (sibling != this && canMergeWith(sibling)) {
                mergeWith(sibling);
                break; // 一次只合并一个
            }
        }
    }
}
