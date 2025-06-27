#include "FoodBall.h"
#include "GoBiggerConfig.h"
#include <QRandomGenerator>
#include <QDebug>

FoodBall::FoodBall(int ballId, const QPointF& position, const Border& border, const Config& config, QGraphicsItem* parent)
    : BaseBall(ballId, position, GoBiggerConfig::FOOD_SCORE, border, FOOD_BALL, parent)
    , m_config(config)
    , m_colorIndex(0)
{
    // 使用GoBigger标准食物分数（固定100分）
    float minScore = GoBiggerConfig::FOOD_MIN_SCORE;
    float maxScore = GoBiggerConfig::FOOD_MAX_SCORE;
    
    float randomScore = minScore + (maxScore - minScore) * QRandomGenerator::global()->generateDouble();
    setScore(randomScore);
    
    generateColorIndex();
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
    return GoBiggerConfig::getStaticFoodColor(m_colorIndex);
}

void FoodBall::generateColorIndex()
{
    // 简单高效：基于ballId生成固定的颜色索引，避免随机数生成开销
    m_colorIndex = ballId() % 4;
}

void FoodBall::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    // 高性能简化绘制：只绘制基本圆形，无渐变、无发光、无装饰效果
    const QColor& ballColor = getBallColor();
    
    // 关闭抗锯齿以提升性能（食物很小，影响不大）
    painter->setRenderHint(QPainter::Antialiasing, false);
    
    // 简单的实心圆，最高效的绘制方式
    painter->setBrush(QBrush(ballColor));
    painter->setPen(Qt::NoPen);  // 无边框，减少绘制开销
    
    const qreal r = radius();
    painter->drawEllipse(QRectF(-r, -r, 2 * r, 2 * r));
}
