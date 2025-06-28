#include "FoodBallData.h"
#include "../../GoBiggerConfig.h"
#include <QRandomGenerator>

FoodBallData::FoodBallData(int ballId, const QPointF& position, const Border& border)
    : BaseBallData(ballId, FOOD_BALL, position, border)
    , m_colorIndex(QRandomGenerator::global()->bounded(4)) // 0-3的随机颜色索引
{
    setScore(GoBiggerConfig::FOOD_SCORE);
    setRadius(GoBiggerConfig::FOOD_RADIUS * GoBiggerConfig::FOOD_VISUAL_SCALE);
}

bool FoodBallData::canEat(const BaseBallData* other) const
{
    // 食物不能吃任何东西
    Q_UNUSED(other)
    return false;
}

void FoodBallData::eat(BaseBallData* other)
{
    // 食物不能吃任何东西
    Q_UNUSED(other)
}

void FoodBallData::updatePhysics(qreal deltaTime)
{
    // 食物是静态的，不需要物理更新
    Q_UNUSED(deltaTime)
}
