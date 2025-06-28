#ifndef CLONEBALLDATA_H
#define CLONEBALLDATA_H

#include "BaseBallData.h"
#include <QVector>

class SporeBallData;

class CloneBallData : public BaseBallData
{
public:
    CloneBallData(int ballId, const QPointF& position, const Border& border, 
                  int teamId = 0, int playerId = 0);

    // 玩家属性
    int teamId() const { return m_teamId; }
    int playerId() const { return m_playerId; }
    
    // 移动相关
    void move(const QVector2D& direction, qreal duration);
    void applyGoBiggerMovement(const QVector2D& playerInput, const QVector2D& centerForce);
    QVector2D moveDirection() const { return m_moveDirection; }
    void setMoveDirection(const QVector2D& dir) { m_moveDirection = dir; }
    
    // 分裂相关
    bool canSplit() const;
    QVector<CloneBallData*> performSplit(const QVector2D& direction);
    QVector<CloneBallData*> performThornsSplit(const QVector2D& direction, int totalPlayerBalls);
    
    // 孢子相关
    bool canEject() const;
    SporeBallData* ejectSpore(const QVector2D& direction);
    
    // 合并相关
    bool canMergeWith(const CloneBallData* other) const;
    void mergeWith(CloneBallData* other);
    bool shouldRigidCollide(const CloneBallData* other) const;
    void rigidCollision(CloneBallData* other);
    
    // 冷却相关
    int framesSinceLastSplit() const { return m_framesSinceLastSplit; }
    void incrementSplitCooldown() { m_framesSinceLastSplit++; }
    void resetSplitCooldown() { m_framesSinceLastSplit = 0; }
    
    // 基类实现
    bool canEat(const BaseBallData* other) const override;
    void eat(BaseBallData* other) override;
    void updatePhysics(qreal deltaTime) override;

private:
    int m_teamId;
    int m_playerId;
    QVector2D m_moveDirection;
    int m_framesSinceLastSplit;
    
    // 内部辅助方法
    void addCenteringForce(const CloneBallData* other);
    void applyCenteringForce(const QVector2D& centerForce);
};

#endif // CLONEBALLDATA_H
