#ifndef FOODBALLDATA_H
#define FOODBALLDATA_H

#include "BaseBallData.h"

class FoodBallData : public BaseBallData
{
public:
    FoodBallData(int ballId, const QPointF& position, const Border& border);

    // 基类实现
    bool canEat(const BaseBallData* other) const override;
    void eat(BaseBallData* other) override;
    void updatePhysics(qreal deltaTime) override;
    
    // 食物特有属性
    int colorIndex() const { return m_colorIndex; }
    void setColorIndex(int index) { m_colorIndex = index; }

private:
    int m_colorIndex;
};

#endif // FOODBALLDATA_H
