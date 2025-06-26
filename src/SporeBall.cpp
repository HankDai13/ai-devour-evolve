#include "SporeBall.h"
#include "CloneBall.h"
#include "GoBiggerConfig.h"
#include <QRandomGenerator>
#include <QGraphicsScene>
#include <QDebug>
#include <QtMath>

SporeBall::SporeBall(int ballId, const QPointF& position, const Border& border, 
                     int teamId, int playerId, const QVector2D& direction, const Config& config, QGraphicsItem* parent)
    : BaseBall(ballId, position, GoBiggerConfig::EJECT_MASS, border, SPORE_BALL, parent)
    , m_config(config)
    , m_teamId(teamId)
    , m_playerId(playerId)
    , m_direction(direction.normalized())
    , m_initialVelocity(GoBiggerConfig::EJECT_SPEED)  // 使用标准的50.0速度
    , m_velocityPiece(QVector2D(0.0f, 0.0f))
    , m_moveFrame(0)
    , m_velocityZeroFrame(GoBiggerConfig::EJECT_VEL_ZERO_FRAME)  // 使用标准的10帧衰减
    , m_remainingLifetime(GoBiggerConfig::SPORE_LIFESPAN)
    , m_framesSinceCreation(0)  // 初始化创建帧计数
    , m_lifetimeTimer(nullptr)
{
    // 设置初始速度，基于GoBigger实现
    QVector2D initialVel = m_direction * m_initialVelocity;
    setVelocity(initialVel);
    m_velocityPiece = initialVel / static_cast<float>(m_velocityZeroFrame); // 每帧减少的速度
    
    qDebug() << "SporeBall created with initial velocity:" << initialVel.length() 
             << "direction:" << m_direction.x() << m_direction.y()
             << "vel_piece:" << m_velocityPiece.length();
    
    initializeTimer();
}

SporeBall::~SporeBall()
{
    if (m_lifetimeTimer) {
        m_lifetimeTimer->stop();
        delete m_lifetimeTimer;
    }
}

void SporeBall::initializeTimer()
{
    m_lifetimeTimer = new QTimer(this);
    connect(m_lifetimeTimer, &QTimer::timeout, this, &SporeBall::updateLifetime);
    m_lifetimeTimer->start(16); // ~60 FPS
}

void SporeBall::move(const QVector2D& direction, qreal duration)
{
    Q_UNUSED(direction)
    
    // 完全按照GoBigger的实现：直接更新位置
    if (m_moveFrame < m_velocityZeroFrame) {
        // GoBigger原版逻辑：self.position = self.position + self.vel * duration
        QVector2D currentVel = velocity();
        
        // 更新位置（GoBigger方式）
        QPointF currentPos = pos();
        QPointF displacement(currentVel.x() * duration, currentVel.y() * duration);
        QPointF newPos = currentPos + displacement;
        setPos(newPos);
        
        qDebug() << "SporeBall move frame:" << m_moveFrame 
                 << "pos from (" << currentPos.x() << "," << currentPos.y() << ")"
                 << "to (" << newPos.x() << "," << newPos.y() << ")"
                 << "displacement:" << displacement.x() << displacement.y()
                 << "vel:" << currentVel.length()
                 << "duration:" << duration;
        
        // 然后减少速度：只减少喷射速度部分，保留继承的速度
        QVector2D newVel = currentVel - m_velocityPiece;
        
        // 确保速度减少的方向是正确的（沿着喷射方向）
        qreal projectionLength = QVector2D::dotProduct(newVel, m_direction);
        if (projectionLength > 0) {
            setVelocity(newVel);
        } else {
            // 如果喷射速度已经完全衰减，保留垂直分量
            QVector2D perpendicularVel = newVel - m_direction * projectionLength;
            setVelocity(perpendicularVel);
        }
        
        m_moveFrame++;
        
        // 检查边界
        checkBorder();
    }
    // 注意：不设置速度为0，保留继承的玩家球速度
}

bool SporeBall::canEat(BaseBall* other) const
{
    Q_UNUSED(other)
    
    // 孢子球不能主动吃其他球
    return false;
}

void SporeBall::eat(BaseBall* other)
{
    Q_UNUSED(other)
    
    // 孢子球不能吃其他球
    qDebug() << "SporeBall cannot eat others";
}

QColor SporeBall::getBallColor() const
{
    // 孢子球使用半透明的团队颜色
    QColor teamColor = getTeamColor(m_teamId);
    teamColor.setAlpha(180); // 半透明效果
    return teamColor;
}

void SporeBall::updatePhysics(qreal deltaTime)
{
    BaseBall::updatePhysics(deltaTime);
    
    // 检查是否需要停止移动
    if (velocity().length() < m_config.minVelocity) {
        setVelocity(QVector2D(0, 0));
    }
}

void SporeBall::updateLifetime()
{
    m_framesSinceCreation++;  // 增加创建后帧计数
    m_remainingLifetime--;
    
    if (m_remainingLifetime <= 0) {
        emit sporeExpired(this);
        remove();
    } else {
        // 随着生存时间减少，透明度也减少
        qreal alpha = static_cast<qreal>(m_remainingLifetime) / m_config.lifetimeFrames;
        QColor color = getBallColor();
        color.setAlpha(static_cast<int>(alpha * 180));
        
        // 更新显示（通过触发重绘）
        update();
    }
}

QColor SporeBall::getTeamColor(int teamId) const
{
    // 与CloneBall使用相同的团队颜色系统
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

// 新的构造函数，可以接收玩家球的速度
SporeBall::SporeBall(int ballId, const QPointF& position, const Border& border, 
                     int teamId, int playerId, const QVector2D& direction, const QVector2D& parentVelocity,
                     const Config& config, QGraphicsItem* parent)
    : BaseBall(ballId, position, GoBiggerConfig::EJECT_MASS, border, SPORE_BALL, parent)
    , m_config(config)
    , m_teamId(teamId)
    , m_playerId(playerId)
    , m_direction(direction.normalized())
    , m_initialVelocity(GoBiggerConfig::EJECT_SPEED)
    , m_velocityPiece(QVector2D(0.0f, 0.0f))
    , m_moveFrame(0)
    , m_velocityZeroFrame(GoBiggerConfig::EJECT_VEL_ZERO_FRAME)
    , m_remainingLifetime(GoBiggerConfig::SPORE_LIFESPAN)
    , m_framesSinceCreation(0)  // 初始化创建帧计数
    , m_lifetimeTimer(nullptr)
{
    // 计算孢子的初始速度：玩家球速度 + 孢子喷射速度
    QVector2D sporeVelocity = m_direction * m_initialVelocity;
    QVector2D totalVelocity = parentVelocity + sporeVelocity;
    
    setVelocity(totalVelocity);
    m_velocityPiece = sporeVelocity / static_cast<float>(m_velocityZeroFrame); // 只有喷射速度部分会衰减
    
    qDebug() << "SporeBall created with parent velocity:" << parentVelocity.length()
             << "spore velocity:" << sporeVelocity.length()
             << "total velocity:" << totalVelocity.length()
             << "direction:" << m_direction.x() << m_direction.y();
    
    initializeTimer();
}
