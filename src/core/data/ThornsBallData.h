#ifndef THORNSBALLDATA_H
#define THORNSBALLDATA_H

#include "BaseBallData.h"

class SporeBallData;

class ThornsBallData : public BaseBallData
{
public:
    ThornsBallData(int ballId, const QPointF& position, const Border& border);

    // 荆棘特有功能
    void eatSpore(SporeBallData* spore);
    bool isMoving() const { return m_isMoving; }
    int colorIndex() const { return m_colorIndex; }
    
    // 基类实现
    bool canEat(const BaseBallData* other) const override;
    void eat(BaseBallData* other) override;
    void updatePhysics(qreal deltaTime) override;

private:
    bool m_isMoving;
    int m_moveFramesLeft;
    int m_colorIndex;
    
    void updateMovement();
};

#endif // THORNSBALLDATA_H
