#ifndef THORNSBALL_H
#define THORNSBALL_H

#include "BaseBall.h"

class ThornsBall : public BaseBall
{
    Q_OBJECT

public:
    struct Config {
        qreal scoreMin = 5.0;
        qreal scoreMax = 15.0;
        qreal damageMultiplier = 0.8;    // 伤害倍数
        int minSplitParts = 5;           // 最小分裂数量
        int maxSplitParts = 10;          // 最大分裂数量
        qreal splitScoreRatio = 0.8;     // 分裂后分数比例
        
        Config() = default;
    };

    ThornsBall(int ballId, const QPointF& position, const Border& border, 
               const Config& config = Config(), QGraphicsItem* parent = nullptr);

    // 重写基类方法
    void move(const QVector2D& direction, qreal duration) override;
    bool canEat(BaseBall* other) const override;
    void eat(BaseBall* other) override;
    
    // 荆棘球特有功能
    void causeCollisionDamage(class CloneBall* ball);
    
signals:
    void thornsCollision(ThornsBall* thorns, class CloneBall* ball);

protected:
    QColor getBallColor() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    Config m_config;
    QColor m_color;
    
    void generateRandomColor();
    void drawThorns(QPainter* painter);
};

#endif // THORNSBALL_H
