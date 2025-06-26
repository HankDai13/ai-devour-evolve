#include "SporeBall.h"
#include "CloneBall.h"
#include <QRandomGenerator>
#include <QGraphicsScene>
#include <QDebug>

SporeBall::SporeBall(int ballId, const QPointF& position, const Border& border, 
                     int teamId, int playerId, const Config& config, QGraphicsItem* parent)
    : BaseBall(ballId, position, GoBiggerConfig::EJECT_MASS, border, SPORE_BALL, parent)
    , m_config(config)
    , m_teamId(teamId)
    , m_playerId(playerId)
    , m_remainingLifetime(GoBiggerConfig::SPORE_LIFESPAN)
    , m_lifetimeTimer(nullptr)
{
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

void SporeBall::setInitialVelocity(const QVector2D& velocity)
{
    // 限制最大速度
    if (velocity.length() > m_config.maxVelocity) {
        setVelocity(velocity.normalized() * m_config.maxVelocity);
    } else {
        setVelocity(velocity);
    }
}

void SporeBall::move(const QVector2D& direction, qreal duration)
{
    Q_UNUSED(direction)
    
    // 孢子球不受外部方向控制，只受物理影响
    // 应用摩擦力
    QVector2D currentVel = velocity();
    if (currentVel.length() > m_config.minVelocity) {
        setVelocity(currentVel * m_config.friction);
    } else {
        setVelocity(QVector2D(0, 0));
    }
    
    // 使用基类的物理移动
    BaseBall::move(QVector2D(0, 0), duration);
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
