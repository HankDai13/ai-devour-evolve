#include "CloneBall.h"
#include "SporeBall.h"
#include "FoodBall.h"
#include <QRandomGenerator>
#include <QGraphicsScene>
#include <QDebug>
#include <QDateTime>
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
    // 简化分裂判定，只检查质量
    return m_mass >= GoBiggerConfig::SPLIT_MIN_MASS;
}

bool CloneBall::canEject() const
{
    // 降低孢子喷射要求，只需要大于孢子质量即可
    bool canEject = m_mass > GoBiggerConfig::EJECT_MASS;
    qDebug() << "Ball" << m_ballId << "canEject check: mass=" << m_mass 
             << "EJECT_MASS=" << GoBiggerConfig::EJECT_MASS 
             << "result=" << canEject;
    return canEject;
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
    
    // 计算分裂位置 - 参考GoBigger: position + direction * (radius * 2)
    QVector2D splitDir = direction.length() > 0.01 ? direction.normalized() : m_moveDirection.normalized();
    if (splitDir.length() < 0.01) {
        splitDir = QVector2D(1, 0); // 默认向右
    }
    
    QPointF newPos = pos() + QPointF(splitDir.x() * radius() * 2.0f, splitDir.y() * radius() * 2.0f);
    
    // 创建新的球
    CloneBall* newBall = new CloneBall(
        m_ballId + 1000, // 临时ID策略
        newPos,
        m_border,
        m_teamId,
        m_playerId,
        m_config
    );
    
    // 设置质量
    setMass(splitMass);
    newBall->setMass(splitMass);
    
    // 应用分裂速度 - 参考GoBigger的分裂速度计算
    float splitVelMagnitude = calculateSplitVelocityFromSplit(radius());
    QVector2D splitVelocity = splitDir * splitVelMagnitude;
    
    // 设置分裂速度 - 两个球相反方向
    applySplitVelocityEnhanced(splitVelocity, splitVelMagnitude, false);
    newBall->applySplitVelocityEnhanced(-splitVelocity, splitVelMagnitude, false);
    
    // 重置分裂计时器
    m_frameSinceLastSplit = 0;
    newBall->m_frameSinceLastSplit = 0;
    
    // 设置分裂关系 - 关键：确保新球也继承移动状态
    newBall->setSplitParent(this);
    newBall->m_moveDirection = m_moveDirection; // 继承移动方向
    newBall->m_fromSplit = true;
    m_fromSplit = true; // 原球也标记为分裂状态
    
    // 确保新球正确初始化定时器和状态
    newBall->initializeTimers(); // 确保计时器初始化
    
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
    
    // 确定孢子方向：如果没有指定方向，使用当前移动方向
    QVector2D sporeDirection;
    if (direction.length() > 0.01) {
        sporeDirection = direction.normalized();
    } else {
        sporeDirection = m_moveDirection.length() > 0.01 ? m_moveDirection.normalized() : QVector2D(1, 0);
    }
    
    // 使用GoBigger标准孢子质量和消耗
    float sporeMass = GoBiggerConfig::EJECT_MASS;
    float massLoss = m_mass * GoBiggerConfig::EJECT_COST_RATIO;
    massLoss = std::max(massLoss, sporeMass); // 至少消耗孢子质量
    
    // 减少自己的质量
    setMass(m_mass - massLoss);
    
    // 计算孢子位置：在玩家球边缘外切，增加距离避免立即重叠
    float sporeRadius = GoBiggerConfig::massToRadius(sporeMass);
    float safeDistance = (radius() + sporeRadius) * 2.0f; // 增加到2倍安全距离
    
    // 添加随机偏移，避免多个孢子重叠在完全相同的位置
    static int sporeCounter = 0;
    sporeCounter++;
    float angleOffset = (sporeCounter % 8) * 45.0f; // 每个孢子偏移45度
    QVector2D offsetDirection = sporeDirection;
    if (sporeCounter > 1) {
        // 计算偏移角度
        float radians = qDegreesToRadians(angleOffset * 0.2f); // 增加偏移幅度
        float cos_val = std::cos(radians);
        float sin_val = std::sin(radians);
        offsetDirection = QVector2D(
            sporeDirection.x() * cos_val - sporeDirection.y() * sin_val,
            sporeDirection.x() * sin_val + sporeDirection.y() * cos_val
        );
    }
    
    QPointF sporePos = pos() + QPointF(offsetDirection.x() * safeDistance, 
                                       offsetDirection.y() * safeDistance);
    
    // 创建孢子球，使用时间戳+计数器确保唯一ID
    int uniqueId = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 1000000) + sporeCounter;
    SporeBall* spore = new SporeBall(
        uniqueId, // 使用更好的唯一ID算法
        sporePos,
        m_border,
        m_teamId,
        m_playerId,
        sporeDirection,  // 孢子方向
        velocity()       // 玩家球当前速度
    );
    
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
        // 使用GoBigger风格的动态速度计算
        float currentRadius = radius();
        float inputRatio = direction.length(); // 输入强度比例
        
        // 计算动态最大速度和加速度
        qreal maxSpeed = GoBiggerConfig::calculateDynamicSpeed(currentRadius, inputRatio);
        qreal dynAccel = GoBiggerConfig::calculateDynamicAcceleration(currentRadius, inputRatio);
        
        // 更直接的速度控制，减少延迟感
        QVector2D targetVelocity = direction.normalized() * maxSpeed;
        
        // 动态加速度控制
        QVector2D accel = (targetVelocity - velocity()) * dynAccel;
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
        
        // 基于半径动态调整速度变化限制
        float maxChange = maxSpeed * (0.1f + 0.2f / currentRadius); // 小球变化更快
        if (velocityDiff.length() > maxChange) {
            velocityDiff = velocityDiff.normalized() * maxChange;
        }
        
        setVelocity(currentVel + velocityDiff);
    } else {
        // 如果没有输入方向，应用更平滑的阻力
        QVector2D currentVel = velocity();
        if (currentVel.length() > 0.1) {
            // 大球减速更慢，小球减速更快
            float dampingFactor = 0.88f + 0.08f / radius();
            setVelocity(currentVel * dampingFactor);
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
    
    // 不能吃同队的球（但可以吃同队的孢子）
    if (other->ballType() == CLONE_BALL) {
        CloneBall* otherClone = static_cast<CloneBall*>(other);
        if (otherClone->teamId() == m_teamId) {
            return false;
        }
    }
    
    // 可以吃孢子球（包括自己的）
    if (other->ballType() == SPORE_BALL) {
        SporeBall* spore = static_cast<SporeBall*>(other);
        qDebug() << "CloneBall" << ballId() << "checking spore" << spore->ballId() 
                 << "player team/id:" << m_teamId << "/" << m_playerId 
                 << "spore team/id:" << spore->teamId() << "/" << spore->playerId();
        return true; // 孢子球可以被任何玩家球吞噬
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
    
    // 应用温和的向心力
    applyCenteringForce();
    
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
    // 统一控制分裂出的所有球 - 移除向心力，改为直接同步移动
    for (CloneBall* child : m_splitChildren) {
        if (child && !child->isRemoved()) {
            child->m_moveDirection = direction.normalized();
            child->updateDirection();
            // 直接同步移动，避免卡顿
            child->move(direction, 0.016);
        }
    }
    
    // 如果自己是子球，也要控制兄弟球
    if (m_splitParent && !m_splitParent->isRemoved()) {
        QVector<CloneBall*> siblings = m_splitParent->getSplitChildren();
        for (CloneBall* sibling : siblings) {
            if (sibling && sibling != this && !sibling->isRemoved()) {
                sibling->m_moveDirection = direction.normalized();
                sibling->updateDirection();
                // 直接同步移动
                sibling->move(direction, 0.016);
            }
        }
        
        // 父球也同步移动
        if (m_splitParent && !m_splitParent->isRemoved()) {
            m_splitParent->m_moveDirection = direction.normalized();
            m_splitParent->updateDirection();
            m_splitParent->move(direction, 0.016);
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
                                
    // 绘制移动方向箭头（基于GoBigger的to_arrow实现）
    if (m_moveDirection.length() > 0.01) {
        drawDirectionArrow(painter, m_moveDirection, ballColor);
    }
}

void CloneBall::drawDirectionArrow(QPainter* painter, const QVector2D& direction, const QColor& color)
{
    // 基于GoBigger的to_arrow函数实现
    const qreal outFactor = 1.2;  // 箭头延伸因子
    const qreal sqrt2_2 = 0.707107; // sqrt(2)/2
    
    QVector2D normalizedDir = direction.normalized();
    qreal x = normalizedDir.x();
    qreal y = normalizedDir.y();
    qreal r = radius();
    
    // 计算箭头的三个点
    QPointF tip(x * outFactor * r, y * outFactor * r);
    QPointF left(-sqrt2_2 * r * (y - x), sqrt2_2 * r * (x + y));
    QPointF right(sqrt2_2 * r * (x + y), sqrt2_2 * r * (y - x));
    
    // 创建箭头多边形
    QPolygonF arrow;
    arrow << tip << left << right;
    
    // 绘制箭头
    QColor arrowColor = color.darker(150);
    arrowColor.setAlpha(180);
    painter->setBrush(QBrush(arrowColor));
    painter->setPen(QPen(arrowColor.darker(130), 2));
    painter->drawPolygon(arrow);
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

bool CloneBall::shouldRigidCollide(CloneBall* other) const
{
    if (!other || other->isRemoved() || this->isRemoved()) {
        return false;
    }
    
    // 必须是同一玩家的球
    if (other->teamId() != m_teamId || other->playerId() != m_playerId) {
        return false;
    }
    
    // 只有分裂后未达到合并时间的球才会刚体碰撞
    return (m_frameSinceLastSplit < m_config.recombineFrame || 
            other->frameSinceLastSplit() < other->m_config.recombineFrame);
}

void CloneBall::rigidCollision(CloneBall* other)
{
    if (!shouldRigidCollide(other) || other->ballId() == m_ballId) {
        return;
    }
    
    // 计算两球间的距离和重叠
    QPointF p = other->pos() - pos();
    qreal distance = std::sqrt(p.x() * p.x() + p.y() * p.y());
    qreal totalRadius = radius() + other->radius();
    
    if (totalRadius > distance && distance > 0.001) {
        // 计算重叠量和推开力
        qreal overlap = totalRadius - distance;
        qreal force = std::min(overlap, overlap / (distance + 1e-8));
        
        // 根据质量比例分配推开距离
        qreal totalMass = m_mass + other->mass();
        qreal myRatio = other->mass() / totalMass;
        qreal otherRatio = m_mass / totalMass;
        
        // 单位向量
        QVector2D pushDirection(p.x() / distance, p.y() / distance);
        
        // 推开两个球
        QPointF myOffset = QPointF(-pushDirection.x() * force * myRatio, 
                                   -pushDirection.y() * force * myRatio);
        QPointF otherOffset = QPointF(pushDirection.x() * force * otherRatio, 
                                      pushDirection.y() * force * otherRatio);
        
        setPos(pos() + myOffset);
        other->setPos(other->pos() + otherOffset);
        
        // 检查边界
        checkBorder();
        other->checkBorder();
    }
}

void CloneBall::addCenteringForce(CloneBall* target)
{
    if (!target || target->isRemoved() || target == this) {
        return;
    }
    
    // 只有在分裂后的重组期间才应用向心力
    if (m_frameSinceLastSplit >= m_config.recombineFrame) {
        return;
    }
    
    // 计算向心力方向（朝向目标球）
    QPointF direction = target->pos() - pos();
    qreal distance = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());
    
    if (distance > 0.001) {
        // 向心力强度：距离越远力越大，但有最大限制
        qreal maxForce = m_config.centerAccWeight;
        qreal forceRatio = std::min(1.0, distance / (radius() * 10)); // 10倍半径内全力
        qreal force = maxForce * forceRatio * 0.02; // 减小力度，避免过于激进
        
        // 单位向量
        QVector2D centeringDirection(direction.x() / distance, direction.y() / distance);
        QVector2D centeringForce = centeringDirection * force;
        
        // 将向心力添加到当前速度
        QVector2D currentVel = velocity();
        setVelocity(currentVel + centeringForce);
    }
}

void CloneBall::applyCenteringForce()
{
    // 只有在分裂后的重组期间才应用向心力
    if (m_frameSinceLastSplit >= m_config.recombineFrame) {
        return;
    }
    
    QVector<CloneBall*> targetBalls;
    
    // 收集所有需要向心力的目标球
    if (m_splitParent && !m_splitParent->isRemoved()) {
        targetBalls.append(m_splitParent);
        // 同时收集兄弟球
        for (CloneBall* sibling : m_splitParent->getSplitChildren()) {
            if (sibling && sibling != this && !sibling->isRemoved()) {
                targetBalls.append(sibling);
            }
        }
    }
    
    // 对子球应用向心力
    for (CloneBall* child : m_splitChildren) {
        if (child && !child->isRemoved()) {
            targetBalls.append(child);
        }
    }
    
    if (targetBalls.isEmpty()) {
        return;
    }
    
    // 计算中心位置（所有分裂球的质心）
    QPointF centerPos(0, 0);
    float totalMass = 0;
    
    for (CloneBall* ball : targetBalls) {
        centerPos += ball->pos() * ball->mass();
        totalMass += ball->mass();
    }
    centerPos += pos() * m_mass;
    totalMass += m_mass;
    
    centerPos /= totalMass;
    
    // 计算向心力
    QPointF direction = centerPos - pos();
    qreal distance = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());
    
    if (distance > radius() * 0.5) { // 只有距离中心超过半个半径才应用向心力
        // 温和的向心力，基于距离
        qreal forceStrength = std::min(0.5, distance / (radius() * 20)); // 更温和的力度
        QVector2D centeringForce(direction.x() / distance * forceStrength, 
                                direction.y() / distance * forceStrength);
        
        // 将向心力添加到当前速度（很小的增量）
        QVector2D currentVel = velocity();
        setVelocity(currentVel + centeringForce * 0.1); // 进一步减小影响
    }
}

void CloneBall::applyGoBiggerMovement(const QVector2D& playerInput, const QVector2D& centerForce)
{
    // GoBigger风格的双重加速度控制，提升速度和丝滑度
    // 参考原版：given_acc (玩家输入) + given_acc_center (向心力)
    
    float currentRadius = radius();
    
    // 1. 处理玩家输入加速度 (given_acc) - 增大系数提升响应速度
    QVector2D givenAcc(0, 0);
    if (playerInput.length() > 0.01) {
        QVector2D normalizedInput = playerInput.length() > 1.0f ? playerInput.normalized() : playerInput;
        givenAcc = normalizedInput * GoBiggerConfig::ACCELERATION_FACTOR * 50.0f; // 增大加速度响应
    }
    
    // 2. 处理向心力加速度 (given_acc_center)  
    QVector2D centerAcc(0, 0);
    if (centerForce.length() > 0.01) {
        QVector2D normalizedCenter = centerForce.length() > 1.0f ? centerForce.normalized() : centerForce;
        // 参考原版：given_acc_center = given_acc_center / self.radius
        centerAcc = normalizedCenter / currentRadius * 15.0f; // 增强向心力
    }
    
    // 3. 计算总加速度
    QVector2D totalAcc = givenAcc + centerAcc;
    
    // 4. 计算最大速度限制（参考原版公式） - 提升速度倍数
    float inputRatio = std::max(playerInput.length(), centerForce.length());
    float maxSpeed = GoBiggerConfig::calculateDynamicSpeed(currentRadius, inputRatio) * 1.5f; // 提升速度倍数
    
    // 5. 更新速度（简化版，类似原版的vel_given更新） - 更快的时间步长
    QVector2D newVelocity = velocity() + totalAcc * 0.02f; // 增大时间步长提升响应
    
    // 6. 限制最大速度
    if (newVelocity.length() > maxSpeed) {
        newVelocity = newVelocity.normalized() * maxSpeed;
    }
    
    // 7. 应用速度
    setVelocity(newVelocity);
    
    // 8. 更新方向（参考原版update_direction） - 只有在有输入时才更新
    if (playerInput.length() > 0.01) {
        m_moveDirection = playerInput; // 确保方向箭头正确显示
        updateDirection();
    }
}
