#include "ThornsBall.h"
#include "CloneBall.h"
#include "SporeBall.h"
#include "GoBiggerConfig.h"
#include <QRandomGenerator>
#include <QPainter>
#include <QPolygonF>
#include <QDebug>
#include <cmath>

ThornsBall::ThornsBall(int ballId, const QPointF& position, const Border& border, 
                       const Config& config, QGraphicsItem* parent)
    : BaseBall(ballId, position, GoBiggerConfig::THORNS_MIN_SCORE, border, THORNS_BALL, parent)
    , m_config(config)
    , m_isMoving(false)
    , m_velocity(0, 0)
    , m_moveFramesLeft(0)
{
    // 生成随机分数
    float minScore = GoBiggerConfig::THORNS_MIN_SCORE;
    float maxScore = GoBiggerConfig::THORNS_MAX_SCORE;
    
    float randomScore = minScore + (maxScore - minScore) * QRandomGenerator::global()->generateDouble();
    setScore(randomScore);
    
    generateRandomColor();
}

void ThornsBall::move(const QVector2D& direction, qreal duration)
{
    // GoBigger荆棘球移动机制：只有吃孢子后才能移动
    if (m_isMoving && m_moveFramesLeft > 0) {
        updateMovement();
        
        // 应用移动
        QPointF currentPos = pos();
        QPointF newPos = currentPos + QPointF(m_velocity.x() * duration, m_velocity.y() * duration);
        
        // 边界检查
        if (m_border.contains(newPos)) {
            setPos(newPos);
        }
        
        m_moveFramesLeft--;
        if (m_moveFramesLeft <= 0) {
            m_isMoving = false;
            m_velocity = QVector2D(0, 0);
        }
    }
}

bool ThornsBall::canEat(BaseBall* other) const
{
    Q_UNUSED(other)
    
    // 荆棘球不能吃其他球，但会对玩家球造成伤害
    return false;
}

void ThornsBall::eat(BaseBall* other)
{
    Q_UNUSED(other)
    
    // 荆棘球不能吃其他球
    qDebug() << "ThornsBall cannot eat others";
}

void ThornsBall::causeCollisionDamage(CloneBall* ball)
{
    if (!ball || ball->isRemoved()) {
        return;
    }
    
    qDebug() << "ThornsBall collision with CloneBall" << ball->ballId() 
             << "Original score:" << ball->score();
    
    // GoBigger标准：如果玩家球分数不足以吃荆棘球，碰撞不产生任何影响
    qDebug() << "Player ball cannot eat thorns ball - no collision effect";
}

QColor ThornsBall::getBallColor() const
{
    return m_color;
}

void ThornsBall::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    // 绘制基础圆形
    painter->setBrush(QBrush(m_color));
    painter->setPen(QPen(m_color.darker(150), 2));
    painter->drawEllipse(QRectF(-radius(), -radius(), 2 * radius(), 2 * radius()));
    
    // 绘制荆棘
    drawThorns(painter);
    
    // 绘制中心高光
    QColor highlightColor = m_color.lighter(150);
    highlightColor.setAlpha(100);
    painter->setBrush(QBrush(highlightColor));
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QRectF(-radius() * 0.3, -radius() * 0.3, radius() * 0.6, radius() * 0.6));
}

void ThornsBall::generateRandomColor()
{
    // 荆棘球使用较暗的颜色
    QList<QColor> thornsColors = {
        QColor(80, 80, 80),    // 深灰
        QColor(60, 40, 20),    // 棕色
        QColor(40, 60, 40),    // 深绿
        QColor(60, 20, 20),    // 深红
        QColor(40, 40, 60),    // 深蓝
        QColor(60, 60, 20),    // 深黄
        QColor(60, 20, 60),    // 深紫
        QColor(20, 60, 60),    // 深青
    };
    
    QRandomGenerator* rng = QRandomGenerator::global();
    m_color = thornsColors[rng->bounded(thornsColors.size())];
}

void ThornsBall::drawThorns(QPainter* painter)
{
    painter->setPen(QPen(m_color.darker(200), 1));
    painter->setBrush(QBrush(m_color.darker(120)));
    
    // 绘制多个荆棘尖刺
    int numThorns = 8 + static_cast<int>(radius() / 5); // 根据大小调整荆棘数量
    qreal angleStep = 2.0 * M_PI / numThorns;
    
    for (int i = 0; i < numThorns; ++i) {
        qreal angle = i * angleStep;
        qreal thornLength = radius() * 0.3; // 荆棘长度
        qreal thornWidth = radius() * 0.1;  // 荆棘宽度
        
        // 计算荆棘的顶点
        qreal baseX = cos(angle) * radius();
        qreal baseY = sin(angle) * radius();
        qreal tipX = cos(angle) * (radius() + thornLength);
        qreal tipY = sin(angle) * (radius() + thornLength);
        
        // 计算荆棘的侧面点
        qreal perpAngle = angle + M_PI / 2;
        qreal sideX1 = baseX + cos(perpAngle) * thornWidth;
        qreal sideY1 = baseY + sin(perpAngle) * thornWidth;
        qreal sideX2 = baseX - cos(perpAngle) * thornWidth;
        qreal sideY2 = baseY - sin(perpAngle) * thornWidth;
        
        // 绘制荆棘三角形
        QPolygonF thorn;
        thorn << QPointF(tipX, tipY)
              << QPointF(sideX1, sideY1)
              << QPointF(sideX2, sideY2);
        
        painter->drawPolygon(thorn);
    }
}

// ============ GoBigger荆棘球特殊功能实现 ============

void ThornsBall::eatSpore(SporeBall* spore)
{
    if (!spore || spore->isRemoved()) return;
    
    qDebug() << "Thorns ball" << ballId() << "eating spore" << spore->ballId();
    
    // 获取孢子的运动方向
    QVector2D sporeVelocity = spore->velocity();
    if (sporeVelocity.length() > 0.1f) {
        applySporeMovement(sporeVelocity.normalized());
    }
    
    // 增加分数
    setScore(score() + spore->score());
    
    // 标记孢子为已移除
    spore->remove();
}

void ThornsBall::applySporeMovement(const QVector2D& sporeDirection)
{
    // GoBigger标准：荆棘球获得10的初速度
    m_velocity = sporeDirection * GoBiggerConfig::THORNS_SPORE_SPEED;
    m_moveFramesLeft = GoBiggerConfig::THORNS_SPORE_DECAY_FRAMES;
    m_isMoving = true;
    
    qDebug() << "Thorns ball" << ballId() << "gained velocity:" 
             << m_velocity.x() << m_velocity.y() << "for" << m_moveFramesLeft << "frames";
}

void ThornsBall::updateMovement()
{
    if (!m_isMoving || m_moveFramesLeft <= 0) return;
    
    // GoBigger标准：速度在20帧内均匀衰减到0
    float decayFactor = static_cast<float>(m_moveFramesLeft) / GoBiggerConfig::THORNS_SPORE_DECAY_FRAMES;
    m_velocity = m_velocity * decayFactor;
}
