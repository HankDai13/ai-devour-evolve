#include "CloneBall.h"
#include "SporeBall.h"
#include "FoodBall.h"
#include "ThornsBall.h"
#include "GoBiggerConfig.h"
#include <QRandomGenerator>
#include <QGraphicsScene>
#include <QDebug>
#include <QDateTime>
#include <cmath>

CloneBall::CloneBall(int ballId, const QPointF& position, const Border& border, int teamId, int playerId, 
                     const Config& config, QGraphicsItem* parent)
    : BaseBall(ballId, position, GoBiggerConfig::CELL_INIT_SCORE, border, CLONE_BALL, parent) // 使用标准初始分数
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
    // 简化分裂判定，只检查分数
    return m_score >= GoBiggerConfig::SPLIT_MIN_SCORE;
}

bool CloneBall::canEject() const
{
    // 使用GoBigger标准：score >= 3200才能喷射孢子
    bool canEject = m_score >= GoBiggerConfig::EJECT_MIN_SCORE;
    qDebug() << "Ball" << m_ballId << "canEject check: score=" << m_score 
             << "EJECT_MIN_SCORE=" << GoBiggerConfig::EJECT_MIN_SCORE 
             << "result=" << canEject;
    return canEject;
}

void CloneBall::setMoveDirection(const QVector2D& direction)
{
    m_moveDirection = direction.normalized();
    updateDirection();
    
    // 🔥 移除统一传播机制，让每个球独立控制
    // propagateMovementToGroup(direction);
    
    // 立即应用移动
    if (direction.length() > 0.01) {
        move(direction, 0.016); // 以16ms为时间步长移动
    }
}

// AI控制接口实现
void CloneBall::setTargetDirection(const QPointF& direction)
{
    QVector2D vec2d(direction.x(), direction.y());
    setMoveDirection(vec2d);
}

QPointF CloneBall::getVelocity() const
{
    return QPointF(m_velocity.x(), m_velocity.y());
}

void CloneBall::split()
{
    if (canSplit()) {
        QVector2D splitDirection = m_moveDirection.length() > 0.01 ? m_moveDirection : QVector2D(1, 0);
        auto newBalls = performSplit(splitDirection);
        if (!newBalls.isEmpty()) {
            emit splitPerformed(this, newBalls);
        }
    }
}

void CloneBall::ejectSpore(const QPointF& direction)
{
    if (canEject()) {
        QVector2D ejectDir(direction.x(), direction.y());
        if (ejectDir.length() < 0.01) {
            ejectDir = m_moveDirection.length() > 0.01 ? m_moveDirection : QVector2D(1, 0);
        }
        SporeBall* spore = ejectSpore(ejectDir);
        if (spore) {
            emit sporeEjected(this, spore);
        }
    }
}

QVector<CloneBall*> CloneBall::performSplit(const QVector2D& direction)
{
    QVector<CloneBall*> newBalls;
    
    if (!canSplit()) {
        return newBalls;
    }
    
    // 计算分裂后的分数 - 使用GoBigger标准
    float splitScore = m_score / 2.0f;
    
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
    
    // 设置分数
    setScore(splitScore);
    newBall->setScore(splitScore);
    
    // GoBigger风格分裂速度：继承原球的移动速度，而不是设置分裂推力
    QVector2D originalVelocity = velocity();
    
    // 设置分裂速度 - 两个球继承相同的移动速度（GoBigger原版机制）
    setVelocity(originalVelocity);
    newBall->setVelocity(originalVelocity);
    
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
    
    // 确定孢子方向：使用传入的方向参数
    QVector2D sporeDirection;
    if (direction.length() > 0.01) {
        sporeDirection = direction.normalized();
    } else {
        // 如果没有指定方向，使用当前移动方向作为备选
        sporeDirection = m_moveDirection.length() > 0.01 ? m_moveDirection.normalized() : QVector2D(1, 0);
    }
    
    // 使用GoBigger标准孢子分数和消耗
    float sporeScore = GoBiggerConfig::EJECT_SCORE;
    float scoreLoss = m_score * GoBiggerConfig::EJECT_COST_RATIO;
    scoreLoss = std::max(scoreLoss, (float)sporeScore); // 至少消耗孢子分数
    
    // 减少自己的分数
    setScore(m_score - scoreLoss);
    
    // 计算孢子位置：在玩家球边缘外切，避免立即重叠
    float sporeRadius = GoBiggerConfig::scoreToRadius(sporeScore);
    float safeDistance = (radius() + sporeRadius) * 1.5f; // 1.5倍安全距离
    
    // 🔥 修复：直接使用指定方向，不添加随机偏移
    QPointF sporePos = pos() + QPointF(sporeDirection.x() * safeDistance, 
                                       sporeDirection.y() * safeDistance);
    
    // 创建孢子球，使用时间戳确保唯一ID
    static int sporeIdCounter = 0;
    sporeIdCounter++;
    int uniqueId = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 1000000) + sporeIdCounter;
    
    SporeBall* spore = new SporeBall(
        uniqueId,
        sporePos,
        m_border,
        m_teamId,
        m_playerId,
        sporeDirection,  // 🔥 直接使用计算好的方向，不做偏移
        velocity()       // 玩家球当前速度
    );
    
    // 添加到场景
    if (scene()) {
        scene()->addItem(spore);
    }
    
    emit sporeEjected(this, spore);
    
    qDebug() << "CloneBall" << ballId() << "ejected spore in direction:" 
             << sporeDirection.x() << sporeDirection.y() 
             << "at position:" << sporePos.x() << sporePos.y();
    
    return spore;
}

void CloneBall::move(const QVector2D& direction, qreal duration)
{
    // 🔥 修复：让配置参数真正生效的GoBigger逻辑
    
    if (direction.length() > 0.01) {
        // 标准化输入方向
        QVector2D normalizedDir = direction.normalized();
        float inputRatio = std::min(direction.length(), 1.0f);
        
        // 使用配置的加速度参数
        float accelWeight = GoBiggerConfig::BASE_SPEED * 0.5f; // 让BASE_SPEED影响加速度
        QVector2D targetAccel = normalizedDir * (accelWeight * inputRatio);
        setAcceleration(targetAccel);
        
        // 更新速度：vel_given = vel_given + acc * duration
        QVector2D currentVel = velocity();
        QVector2D newVel = currentVel + targetAccel * duration;
        
        // 🔥 让BASE_SPEED真正影响最大速度
        float currentRadius = radius();
        float baseMaxSpeed = (2.35f + 5.66f / currentRadius) * inputRatio;
        float maxSpeed = baseMaxSpeed * (GoBiggerConfig::BASE_SPEED / 400.0f); // 以400为基准缩放
        
        // format_vector: 限制速度不超过最大值
        if (newVel.length() > maxSpeed) {
            newVel = newVel.normalized() * maxSpeed;
        }
        
        setVelocity(newVel);
    } else {
        // 没有输入时保持当前速度，添加少量阻力
        QVector2D currentVel = velocity();
        setVelocity(currentVel * 0.98f);
        setAcceleration(QVector2D(0, 0));
    }
    
    // 🔥 GoBigger风格分裂速度处理
    if (m_splitVelocity.length() > 0.1) {
        // 分裂速度和移动速度相加
        QVector2D totalVel = velocity() + m_splitVelocity;
        setVelocity(totalVel);
        
        // 分裂速度衰减：按帧数逐渐减少
        m_splitVelocity -= m_splitVelocityPiece;
        
        // 衰减完成后清零
        if (m_splitVelocity.length() < 0.1) {
            m_splitVelocity = QVector2D(0, 0);
        }
        
        // 更新分裂帧计数
        m_splitFrame++;
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
        // 特殊处理：吃荆棘球会触发特殊分裂
        if (other->ballType() == BaseBall::THORNS_BALL) {
            ThornsBall* thorns = static_cast<ThornsBall*>(other);
            
            qDebug() << "CloneBall" << ballId() << "eating thorns ball" << thorns->ballId() 
                     << "- triggering special split";
            
            // 先增加分数
            BaseBall::eat(other);
            
            // 触发荆棘分裂
            // 注意：这里需要通过信号机制来获取准确的玩家球数量
            // 暂时发送信号，让GameManager处理
            emit thornsEaten(this, thorns);
        } else {
            BaseBall::eat(other);
            qDebug() << "CloneBall" << ballId() << "ate" << other->ballId() << "gaining score:" << other->score();
        }
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
    // 🔥 如果球已被移除，立即停止所有移动和更新
    if (isRemoved()) {
        if (m_movementTimer) {
            m_movementTimer->stop();
        }
        if (m_decayTimer) {
            m_decayTimer->stop();
        }
        return;
    }
    
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
    // 🔥 如果球已被移除，停止分数衰减
    if (isRemoved()) {
        if (m_decayTimer) {
            m_decayTimer->stop();
        }
        return;
    }
    
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
    if (m_score > GoBiggerConfig::DECAY_START_SCORE) {
        float decay = m_score * GoBiggerConfig::DECAY_RATE;
        setScore(std::max((float)GoBiggerConfig::CELL_MIN_SCORE, m_score - decay));
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
    
    // 🔥 绘制队伍字母标识（在球中心）
    QChar teamLetter = GoBiggerConfig::getTeamLetter(m_teamId);
    QFont font("Arial", static_cast<int>(radius() * 0.6)); // 字体大小基于球半径
    font.setBold(true);
    
    painter->setFont(font);
    painter->setPen(QPen(Qt::white, 2)); // 白色字体，2像素描边
    
    // 计算文字位置（居中）
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(teamLetter);
    QPointF textPos(-textRect.width() / 2.0, textRect.height() / 2.0 - 2);
    
    // 先绘制黑色描边
    painter->setPen(QPen(Qt::black, 3));
    painter->drawText(textPos, teamLetter);
    
    // 再绘制白色字母
    painter->setPen(QPen(Qt::white, 1));
    painter->drawText(textPos, teamLetter);
                                
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
    
    bool canMerge = distance <= mergeDistance;
    
    // 只在成功合并时打印日志，减少输出
    if (canMerge) {
        qDebug() << "Ball" << m_ballId << "can merge with Ball" << other->ballId() 
                 << "distance:" << distance << "required:" << mergeDistance;
    }
    
    return canMerge;
}

void CloneBall::mergeWith(CloneBall* other)
{
    if (!canMergeWith(other)) {
        return;
    }
    
    qDebug() << "🔗 Ball" << m_ballId << "merging with ball" << other->ballId();
    
    // 🔥 在合并前发出信号通知AI
    emit mergePerformed(this, other);
    
    // 合并分数
    float combinedScore = m_score + other->score();
    setScore(combinedScore);
    
    // 合并速度(加权平均)
    QVector2D combinedVelocity = (m_velocity * m_score + other->velocity() * other->score()) / combinedScore;
    setVelocity(combinedVelocity);
    
    // 重置分裂计时器
    m_frameSinceLastSplit = 0;
    
    // 🔥 彻底移除被合并的球 - 多重保险
    other->remove();
    
    // 🔥 强制断开所有连接，避免悬空指针
    other->setVelocity(QVector2D(0, 0)); // 停止移动
    other->setVisible(false); // 隐藏
    
    // 从分裂关系中移除
    m_splitChildren.removeOne(other);
    if (other->getSplitParent() == this) {
        other->setSplitParent(nullptr);
    }
    
    // 🔥 如果还在场景中，强制移除
    if (other->scene() && other->scene()->items().contains(other)) {
        other->scene()->removeItem(other);
        qDebug() << "Force removed ball" << other->ballId() << "from scene";
    }
    
    qDebug() << "Ball" << m_ballId << "merged with ball" << other->ballId() 
             << "new score:" << combinedScore;
}

void CloneBall::checkForMerge()
{
    int mergeDelayFrames = GoBiggerConfig::MERGE_DELAY * 60; // 20秒 * 60帧
    
    // 如果刚好过了冷却期，打印调试信息
    if (m_frameSinceLastSplit == mergeDelayFrames) {
        qDebug() << "Ball" << m_ballId << "merge cooldown ended, checking for auto-merge";
    }
    
    // 检查与所有子球的合并
    for (CloneBall* child : m_splitChildren) {
        if (child && !child->isRemoved() && canMergeWith(child)) {
            qDebug() << "Ball" << m_ballId << "auto-merging with child" << child->ballId();
            mergeWith(child);
            return; // 一次只合并一个，下次更新时继续
        }
    }
    
    // 如果自己是子球，检查与父球的合并
    if (m_splitParent && !m_splitParent->isRemoved() && canMergeWith(m_splitParent)) {
        qDebug() << "Ball" << m_ballId << "auto-merging with parent" << m_splitParent->ballId();
        m_splitParent->mergeWith(this);
        return; // 自己被合并了，直接返回
    }
    
    // 检查与所有兄弟球的合并（同一父球的子球）
    if (m_splitParent) {
        QVector<CloneBall*> siblings = m_splitParent->getSplitChildren();
        for (CloneBall* sibling : siblings) {
            if (sibling && sibling != this && !sibling->isRemoved() && canMergeWith(sibling)) {
                qDebug() << "Ball" << m_ballId << "auto-merging with sibling" << sibling->ballId();
                mergeWith(sibling);
                return; // 一次只合并一个
            }
        }
    }
    
    // 额外检查：对于没有明确父子关系但同属一个玩家的球
    // 这解决了多次分裂后分身球关系复杂的情况
    // TODO: 这里需要从GameManager获取同玩家的所有球进行检查
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
    int mergeDelayFrames = GoBiggerConfig::MERGE_DELAY * 60; // 20秒 * 60帧
    return (m_frameSinceLastSplit < mergeDelayFrames || 
            other->frameSinceLastSplit() < mergeDelayFrames);
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
        
        // 根据分数比例分配推开距离
        qreal totalScore = m_score + other->score();
        qreal myRatio = other->score() / totalScore;
        qreal otherRatio = m_score / totalScore;
        
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
    int mergeDelayFrames = GoBiggerConfig::MERGE_DELAY * 60; // 20秒 * 60帧
    if (m_frameSinceLastSplit >= mergeDelayFrames) {
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
    int mergeDelayFrames = GoBiggerConfig::MERGE_DELAY * 60; // 20秒 * 60帧
    if (m_frameSinceLastSplit >= mergeDelayFrames) {
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
    
    // 计算质心位置（加权平均，大球权重更大）
    QPointF centerPos(0, 0);
    float totalWeight = 0;
    
    for (CloneBall* ball : targetBalls) {
        float weight = ball->score(); // 使用分数作为权重
        centerPos += ball->pos() * weight;
        totalWeight += weight;
    }
    centerPos += pos() * m_score;
    totalWeight += m_score;
    
    centerPos /= totalWeight;
    
    // 计算到质心的距离向量
    QVector2D toCenter = QVector2D(centerPos - pos());
    float distance = toCenter.length();
    
    // 只有当距离超过最小阈值时才应用向心力
    float minDistance = radius() * 1.5f; // 球半径的1.5倍
    if (distance > minDistance) {
        // GoBigger风格的非线性向心力计算
        // 力度随距离非线性增加，但有最大限制
        float normalizedDistance = std::min(1.0f, (distance - minDistance) / (radius() * 10.0f));
        
        // 使用平滑的非线性函数 (类似easing function)
        float easeInOut = normalizedDistance * normalizedDistance * (3.0f - 2.0f * normalizedDistance);
        
        // 计算力度，随时间衰减（合并时间越近，向心力越强）
        float timeDecay = 1.0f - (float)m_frameSinceLastSplit / mergeDelayFrames;
        float maxForce = 0.8f * timeDecay; // 最大向心力随时间递减
        
        float forceStrength = maxForce * easeInOut;
        
        // 计算向心力向量
        QVector2D centeringForce = toCenter.normalized() * forceStrength;
        
        // 使用GoBigger的标准加速度系统应用向心力
        applyGoBiggerMovement(QVector2D(0, 0), centeringForce);
    }
}

void CloneBall::applyGoBiggerMovement(const QVector2D& playerInput, const QVector2D& centerForce)
{
    // GoBigger风格的双重加速度控制，优化向心力平衡
    // 参考原版：given_acc (玩家输入) + given_acc_center (向心力)
    
    float currentRadius = radius();
    
    // 1. 处理玩家输入加速度 (given_acc) - 使用GoBigger标准参数
    QVector2D givenAcc(0, 0);
    if (playerInput.length() > 0.01) {
        QVector2D normalizedInput = playerInput.length() > 1.0f ? playerInput.normalized() : playerInput;
        givenAcc = normalizedInput * 30.0f; // GoBigger标准acc_weight=30
    }
    
    // 2. 处理向心力加速度 (given_acc_center) - 优化为更平滑的力
    QVector2D centerAcc(0, 0);
    if (centerForce.length() > 0.01) {
        QVector2D normalizedCenter = centerForce.length() > 1.0f ? centerForce.normalized() : centerForce;
        
        // 使用GoBigger原版的向心力公式，但优化参数避免抖动
        // 原版: given_acc_center = given_acc_center / self.radius
        float centerWeight = 8.0f / std::max(currentRadius, 10.0f); // 半径越大，向心力越小
        centerAcc = normalizedCenter * centerWeight;
        
        // 添加速度衰减，避免振荡
        float currentSpeed = velocity().length();
        if (currentSpeed > 50.0f) { // 如果速度过快，减弱向心力
            centerAcc *= (50.0f / currentSpeed);
        }
    }
    
    // 3. 计算总加速度
    QVector2D totalAcc = givenAcc + centerAcc;
    
    // 4. 计算最大速度限制（使用GoBigger原版公式）
    float inputRatio = std::max(playerInput.length(), centerForce.length());
    float maxSpeed = GoBiggerConfig::calculateDynamicSpeed(currentRadius, inputRatio);
    
    // 5. 更新速度（使用标准时间步长）
    QVector2D newVelocity = velocity() + totalAcc * 0.05f; // 标准帧时间
    
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

QVector<CloneBall*> CloneBall::performThornsSplit(const QVector2D& direction, int totalPlayerBalls)
{
    QVector<CloneBall*> newBalls;
    
    // GoBigger荆棘分裂逻辑：
    // 1. 计算需要分裂出的新球数量，使总数不超过16个
    int maxNewBalls = GoBiggerConfig::THORNS_SPLIT_MAX_COUNT;  // 最多分裂出10个新球
    int maxTotalBalls = GoBiggerConfig::MAX_SPLIT_COUNT;       // 总球数上限16个
    
    // 确定实际分裂数量
    int availableSlots = maxTotalBalls - totalPlayerBalls;
    int actualNewBalls = std::min(maxNewBalls, availableSlots);
    
    if (actualNewBalls <= 0) {
        qDebug() << "Cannot split from thorns: already at max ball count";
        return newBalls;
    }
    
    qDebug() << "Thorns split: total balls=" << totalPlayerBalls 
             << "max new=" << maxNewBalls 
             << "actual new=" << actualNewBalls;
    
    // 2. 计算分数分配
    float totalScore = m_score;
    float newBallScore = std::min(static_cast<float>(GoBiggerConfig::THORNS_SPLIT_MAX_SCORE), 
                                  totalScore / (actualNewBalls + 1)); // +1包括原球
    
    // 原球保留剩余分数
    float remainingScore = totalScore - (newBallScore * actualNewBalls);
    setScore(remainingScore);
    
    // 3. 确定分裂方向
    QVector2D splitDir = direction.length() > 0.01 ? direction.normalized() : QVector2D(1, 0);
    
    // 4. 创建新球，按照GoBigger原版：均匀分布在圆周上
    for (int i = 0; i < actualNewBalls; ++i) {
        // 🔥 GoBigger原版荆棘分裂：均匀分布在圆周上
        float angle = 2.0f * M_PI * (i + 1) / actualNewBalls; // 从i+1开始避免重叠
        
        // 计算新球半径用于分离距离
        float newBallRadius = GoBiggerConfig::scoreToRadius(newBallScore);
        
        // GoBigger风格：新球位置 = 原球位置 + (原球半径 + 新球半径) * 方向
        float separationDistance = radius() + newBallRadius;
        QVector2D offset(std::cos(angle) * separationDistance, 
                        std::sin(angle) * separationDistance);
        QPointF newPos = pos() + QPointF(offset.x(), offset.y());
        
        // 创建新球
        CloneBall* newBall = new CloneBall(
            m_ballId + 1000 + i, // 临时ID策略
            newPos,
            m_border,
            m_teamId,
            m_playerId,
            m_config
        );
        
        newBall->setScore(newBallScore);
        newBall->m_fromThorns = true; // 标记为荆棘分裂
        newBall->m_frameSinceLastSplit = 0; // 重置冷却计数器
        
        // 关键修复：设置分裂关系，让荆棘分裂的球能够相互合并
        newBall->setSplitParent(this);  // 设置父球关系
        m_splitChildren.append(newBall); // 添加到子球列表
        
        // 🔥 GoBigger原版荆棘分裂速度：弹出动画 + 原速度继承
        QVector2D splitDirection(std::cos(angle), std::sin(angle));
        
        // 计算荆棘分裂的弹出速度 (基于GoBigger原版公式)
        float splitSpeed = GoBiggerConfig::calcSplitVelInitFromThorns(newBall->radius());
        QVector2D splitVelocity = splitDirection * splitSpeed;
        
        // 新球速度 = 原球移动速度 + 弹出速度
        QVector2D originalVelocity = velocity();
        newBall->setVelocity(originalVelocity + splitVelocity);
        
        // 应用分裂速度系统 (会逐渐衰减到原球速度)
        newBall->applySplitVelocityEnhanced(splitDirection, splitSpeed, true);
        
        newBalls.append(newBall);
    }
    
    // 原球也重置冷却计数器
    m_frameSinceLastSplit = 0;

    qDebug() << "Thorns split completed: created" << newBalls.size() 
             << "new balls with score" << newBallScore 
             << "each, original ball score:" << m_score;

    if (!newBalls.isEmpty()) {
        emit splitPerformed(this, newBalls);
    }
    
    return newBalls;
}

void CloneBall::remove()
{
    // 🔥 立即停止所有定时器，防止"尸体漂移"
    if (m_movementTimer) {
        m_movementTimer->stop();
    }
    if (m_decayTimer) {
        m_decayTimer->stop();
    }
    
    // 清除移动方向，确保球完全停止
    m_moveDirection = QVector2D(0, 0);
    m_velocity = QVector2D(0, 0);
    
    // 🔥 立即从场景中移除，防止"尸体"残留
    if (scene()) {
        scene()->removeItem(this);
        qDebug() << "CloneBall" << ballId() << "removed from scene";
    }
    
    // 调用基类的remove函数
    BaseBall::remove();
    
    qDebug() << "CloneBall" << ballId() << "removed and all timers stopped";
}
