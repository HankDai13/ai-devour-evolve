#ifndef SPOREBALLDATA_H
#define SPOREBALLDATA_H

#include "BaseBallData.h"

class SporeBallData : public BaseBallData
{
public:
    SporeBallData(int ballId, const QPointF& position, const Border& border, 
                  int teamId = 0, int playerId = 0);

    // 玩家属性
    int teamId() const { return m_teamId; }
    int playerId() const { return m_playerId; }
    
    // 孢子特有属性
    int lifespan() const { return m_lifespan; }
    bool canBeEaten() const { return m_framesSinceCreation >= 10; } // 10帧后才能被吃
    
    // 基类实现
    bool canEat(const BaseBallData* other) const override;
    void eat(BaseBallData* other) override;
    void updatePhysics(qreal deltaTime) override;

private:
    int m_teamId;
    int m_playerId;
    int m_lifespan;
    int m_framesSinceCreation;
    
    void updateLifespan();
};

#endif // SPOREBALLDATA_H
