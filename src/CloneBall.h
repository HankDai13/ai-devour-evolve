#ifndef CLONEBALL_H
#define CLONEBALL_H

#include "BaseBall.h"
#include <QTimer>
#include <QVector>

class SporeBall; // 前向声明
class ThornsBall; // 前向声明

class CloneBall : public BaseBall
{
    Q_OBJECT

public:
    struct Config {
        qreal accWeight = 100.0;           // 最大加速度
        qreal velMax = 20.0;               // 最大速度
        qreal scoreInit = 10.0;            // 初始分数
        int partNumMax = 16;               // 最大分裂数量
        int onThornsPartNum = 10;          // 碰到荆棘时的分裂数量
        qreal onThornsPartScoreMax = 3.0;  // 碰到荆棘时分裂部分的最大分数
        qreal splitScoreMin = 5.0;         // 可分裂的最小分数 (增加了)
        qreal ejectScoreMin = 3.0;         // 可喷射孢子的最小分数 (增加了)
        int recombineFrame = 320;          // 分裂球重新结合的时间（帧）
        int splitVelZeroFrame = 40;        // 分裂速度衰减到零的时间（帧）
        qreal scoreDecayMin = 26.0;        // 开始衰减的最小分数
        qreal scoreDecayRatePerFrame = 0.00005; // 每帧的分数衰减率
        qreal centerAccWeight = 10.0;      // 中心加速度权重
        
        Config() = default;
    };

    CloneBall(int ballId, const QPointF& position, const Border& border, int teamId, int playerId, 
              const Config& config = Config(), QGraphicsItem* parent = nullptr);
    
    ~CloneBall();

    // 获取属性
    int teamId() const { return m_teamId; }
    int playerId() const { return m_playerId; }
    bool canSplit() const;
    bool canEject() const;
    int frameSinceLastSplit() const { return m_frameSinceLastSplit; }
    
    // 玩家操作
    void setMoveDirection(const QVector2D& direction);
    // AI控制接口
    void setTargetDirection(const QPointF& direction);
    QPointF getVelocity() const;
    void split(); // 简化的分裂接口
    void ejectSpore(const QPointF& direction); // 简化的孢子喷射接口
    void applyGoBiggerMovement(const QVector2D& playerInput, const QVector2D& centerForce); // 新增：GoBigger风格移动
    QVector<CloneBall*> performSplit(const QVector2D& direction);
    QVector<CloneBall*> performThornsSplit(const QVector2D& direction, int totalPlayerBalls); // 新增：吃荆棘球后的特殊分裂
    SporeBall* ejectSpore(const QVector2D& direction);
    
    // 合并机制
    bool canMergeWith(CloneBall* other) const;
    void mergeWith(CloneBall* other);
    void checkForMerge();
    
    // 分裂球刚体碰撞 - 新增
    bool shouldRigidCollide(CloneBall* other) const;
    void rigidCollision(CloneBall* other);
    
    // 重写基类方法
    void move(const QVector2D& direction, qreal duration) override;
    bool canEat(BaseBall* other) const override;
    void eat(BaseBall* other) override;
    void remove() override;  // 🔥 重写remove函数以停止定时器

signals:
    void splitPerformed(CloneBall* originalBall, const QVector<CloneBall*>& newBalls);
    void sporeEjected(CloneBall* ball, SporeBall* spore);
    void thornsEaten(CloneBall* ball, ThornsBall* thorns); // 新增：吃荆棘球信号
    void mergePerformed(CloneBall* survivingBall, CloneBall* mergedBall); // 新增：合并信号

protected:
    QColor getBallColor() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void updatePhysics(qreal deltaTime) override;

private slots:
    void updateMovement();
    void updateScoreDecay();

private:
    Config m_config;
    int m_teamId;
    int m_playerId;
    
    // 移动相关
    QVector2D m_moveDirection;
    QVector2D m_splitVelocity;
    QVector2D m_splitVelocityPiece;
    
    // 分裂相关
    int m_splitFrame;
    int m_frameSinceLastSplit;
    bool m_fromSplit;
    bool m_fromThorns;
    
    // 分裂统一控制
    CloneBall* m_splitParent;        // 分裂来源的父球
    QVector<CloneBall*> m_splitChildren; // 分裂出的子球
    
    // 定时器
    QTimer* m_movementTimer;
    QTimer* m_decayTimer;
    
    // 初始化
    void initializeTimers();
    void updateDirection();
    
    // 分裂相关计算
    qreal calculateSplitVelocityFromSplit(qreal radius) const;
    qreal calculateSplitVelocityFromThorns(qreal radius) const;
    void applySplitVelocity(const QVector2D& direction, bool fromThorns = false);
    void applySplitVelocityEnhanced(const QVector2D& direction, qreal velocity, bool fromThorns = false);
    
    // 分裂统一控制
    void setSplitParent(CloneBall* parent) { m_splitParent = parent; }
    CloneBall* getSplitParent() const { return m_splitParent; }
    QVector<CloneBall*> getSplitChildren() const { return m_splitChildren; }
    void propagateMovementToGroup(const QVector2D& direction);
    void addCenteringForce(CloneBall* target); // 新增：向心力方法
    void applyCenteringForce(); // 新增：应用向心力到自身
    
    // 得分衰减
    void applyScoreDecay();
    
    // 团队颜色
    QColor getTeamColor(int teamId) const;
    
    // 方向箭头绘制
    void drawDirectionArrow(QPainter* painter, const QVector2D& direction, const QColor& color);
};

#endif // CLONEBALL_H
