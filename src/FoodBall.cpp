#include "FoodBall.h"
#include "GoBiggerConfig.h"
#include <QRandomGenerator>
#include <QDebug>

FoodBall::FoodBall(int ballId, const QPointF& position, const Border& border, const Config& config, QGraphicsItem* parent)
    : BaseBall(ballId, position, GoBiggerConfig::FOOD_SCORE, border, FOOD_BALL, parent)
    , m_config(config)
{
    // 使用GoBigger标准食物分数（固定100分）
    float minScore = GoBiggerConfig::FOOD_MIN_SCORE;
    float maxScore = GoBiggerConfig::FOOD_MAX_SCORE;
    
    float randomScore = minScore + (maxScore - minScore) * QRandomGenerator::global()->generateDouble();
    setScore(randomScore);
    
    qDebug() << "FoodBall created with score:" << score() << "radius:" << radius();
    
    generateRandomColor();
}

void FoodBall::move(const QVector2D& direction, qreal duration)
{
    Q_UNUSED(direction)
    Q_UNUSED(duration)
    
    // 食物球不能移动
    qDebug() << "FoodBall cannot move";
}

bool FoodBall::canEat(BaseBall* other) const
{
    Q_UNUSED(other)
    
    // 食物球不能吃其他球
    return false;
}

void FoodBall::eat(BaseBall* other)
{
    Q_UNUSED(other)
    
    // 食物球不能吃其他球
    qDebug() << "FoodBall cannot eat others";
}

QColor FoodBall::getBallColor() const
{
    return m_color;
}

void FoodBall::generateRandomColor()
{
    // 生成随机的鲜艳颜色作为食物
    QRandomGenerator* rng = QRandomGenerator::global();
    
    // 预定义一些食物颜色 - 更加鲜艳和多样
    QList<QColor> foodColors = {
        QColor(255, 80, 80),   // 亮红色
        QColor(80, 255, 80),   // 亮绿色  
        QColor(80, 80, 255),   // 亮蓝色
        QColor(255, 255, 80),  // 亮黄色
        QColor(255, 80, 255),  // 亮品红
        QColor(80, 255, 255),  // 亮青色
        QColor(255, 150, 80),  // 亮橙色
        QColor(150, 80, 255),  // 亮紫色
        QColor(255, 200, 80),  // 金黄色
        QColor(80, 255, 150),  // 春绿色
        QColor(255, 80, 150),  // 粉红色
        QColor(200, 255, 80)   // 青绿色
    };
    
    m_color = foodColors[rng->bounded(foodColors.size())];
}

void FoodBall::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    // 食物球的特殊绘制 - 更小更精致
    QColor ballColor = getBallColor();
    
    // 绘制外圈发光效果
    QColor glowColor = ballColor;
    glowColor.setAlpha(60);
    painter->setBrush(QBrush(glowColor));
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QRectF(-radius() * 1.2, -radius() * 1.2, 
                                2.4 * radius(), 2.4 * radius()));
    
    // 绘制主体
    QRadialGradient gradient(0, 0, radius());
    gradient.setColorAt(0, ballColor.lighter(180));
    gradient.setColorAt(0.6, ballColor);
    gradient.setColorAt(1, ballColor.darker(130));
    
    painter->setBrush(QBrush(gradient));
    painter->setPen(QPen(ballColor.darker(150), 1));
    painter->drawEllipse(QRectF(-radius(), -radius(), 2 * radius(), 2 * radius()));
    
    // 绘制闪烁的小高光点
    QColor sparkleColor = QColor(255, 255, 255, 180);
    painter->setBrush(QBrush(sparkleColor));
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QRectF(-radius() * 0.3, -radius() * 0.3, 
                                radius() * 0.4, radius() * 0.4));
    
    // 绘制小装饰点
    painter->setBrush(QBrush(ballColor.lighter(150)));
    painter->drawEllipse(QRectF(radius() * 0.2, radius() * 0.2, 
                                radius() * 0.3, radius() * 0.3));
}
