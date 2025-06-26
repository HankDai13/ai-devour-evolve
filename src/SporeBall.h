#ifndef SPOREBALL_H
#define SPOREBALL_H

#include "BaseBall.h"
#include <QTimer>

class SporeBall : public BaseBall
{
    Q_OBJECT

public:
    struct Config {
        qreal scoreMin = 0.3;
        qreal scoreMax = 1.0;
        qreal maxVelocity = 50.0;      // 最大速度
        qreal friction = 0.98;         // 摩擦力系数
        int lifetimeFrames = 600;      // 生存时间（帧）
        qreal minVelocity = 0.5;       // 最小速度阈值
        
        Config() = default;
    };

    SporeBall(int ballId, const QPointF& position, const Border& border, 
              int teamId, int playerId, const QVector2D& direction, const Config& config = Config(), QGraphicsItem* parent = nullptr);
    
    // 重载构造函数，可以接收玩家球的当前速度
    SporeBall(int ballId, const QPointF& position, const Border& border, 
              int teamId, int playerId, const QVector2D& direction, const QVector2D& parentVelocity, 
              const Config& config = Config(), QGraphicsItem* parent = nullptr);
    
    ~SporeBall();

    // 获取属性
    int teamId() const { return m_teamId; }
    int playerId() const { return m_playerId; }
    int remainingLifetime() const { return m_remainingLifetime; }
    QVector2D direction() const { return m_direction; }
    bool canBeEaten() const { return m_framesSinceCreation > 3; } // 3帧后才能被吞噬
    
    // 重写基类方法
    void move(const QVector2D& direction, qreal duration) override;
    bool canEat(BaseBall* other) const override;
    void eat(BaseBall* other) override;

signals:
    void sporeExpired(SporeBall* spore);

protected:
    QColor getBallColor() const override;
    void updatePhysics(qreal deltaTime) override;

private slots:
    void updateLifetime();

private:
    Config m_config;
    int m_teamId;
    int m_playerId;
    QVector2D m_direction;          // 孢子移动方向
    qreal m_initialVelocity;        // 初始速度
    QVector2D m_velocityPiece;      // 每帧减少的速度
    int m_moveFrame;                // 移动帧计数
    int m_velocityZeroFrame;        // 速度衰减到0的帧数
    int m_remainingLifetime;
    int m_framesSinceCreation;      // 创建后经过的帧数
    QTimer* m_lifetimeTimer;
    
    void initializeTimer();
    QColor getTeamColor(int teamId) const;
};

#endif // SPOREBALL_H
