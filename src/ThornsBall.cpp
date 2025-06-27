#include "ThornsBall.h"
#include "CloneBall.h"
#include "GoBiggerConfig.h"
#include <QRandomGenerator>
#include <QPainter>
#include <QPolygonF>
#include <QDebug>
#include <cmath>

ThornsBall::ThornsBall(int ballId, const QPointF& position, const Border& border, 
                       const Config& config, QGraphicsItem* parent)
    : BaseBall(ballId, position, GoBiggerConfig::THORNS_MIN_MASS, border, THORNS_BALL, parent)
    , m_config(config)
{
    // 生成随机质量
    float minMass = GoBiggerConfig::THORNS_MIN_MASS;
    float maxMass = GoBiggerConfig::THORNS_MAX_MASS;
    
    float randomMass = minMass + (maxMass - minMass) * QRandomGenerator::global()->generateDouble();
    setMass(randomMass);
    
    generateRandomColor();
}

void ThornsBall::move(const QVector2D& direction, qreal duration)
{
    Q_UNUSED(direction)
    Q_UNUSED(duration)
    
    // 荆棘球不能移动
    qDebug() << "ThornsBall cannot move";
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
             << "Original mass:" << ball->mass();
    
    // 计算伤害 - 降低伤害比例
    float damage = ball->mass() * GoBiggerConfig::THORNS_DAMAGE_RATIO; // 使用配置伤害比例
    float newMass = ball->mass() - damage;
    
    // 提高移除阈值，防止直接消失
    if (newMass < GoBiggerConfig::CELL_MIN_MASS) { 
        // 如果质量太低，设置为最小值而不是直接移除
        newMass = GoBiggerConfig::CELL_MIN_MASS;
        qDebug() << "Ball mass set to minimum by thorns";
    }
    
    // 减少质量
    ball->setMass(newMass);
    qDebug() << "Ball damaged by thorns, new mass:" << newMass;
    
    // 如果球足够大，触发分裂
    if (ball->mass() > GoBiggerConfig::SPLIT_MIN_MASS * 2) { // 调整分裂阈值
        // 计算分裂数量
        QRandomGenerator* rng = QRandomGenerator::global();
        int splitParts = rng->bounded(2, 4); // 分裂成2-3个部分
        
        // 计算每个分裂部分的质量
        float splitMass = ball->mass() / splitParts;
        
        // 设置原球的新质量
        ball->setMass(splitMass);
        
        qDebug() << "Ball force-split by thorns into" << splitParts << "parts";
        
        // 发送信号通知需要分裂
        emit thornsCollision(this, ball);
    }
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
