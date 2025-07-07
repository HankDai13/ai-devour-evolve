#ifndef FOODBALL_H
#define FOODBALL_H

#include "BaseBall.h"

class FoodBall : public BaseBall
{
    Q_OBJECT

public:
    struct Config {
        qreal scoreMin = 0.2;
        qreal scoreMax = 1.0;
        
        Config() = default;
    };

    FoodBall(int ballId, const QPointF& position, const Border& border, const Config& config = Config(), QGraphicsItem* parent = nullptr);
    
    // 重写基类方法
    void move(const QVector2D& direction, qreal duration) override;
    bool canEat(BaseBall* other) const override;
    void eat(BaseBall* other) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
protected:
    QColor getBallColor() const override;

private:
    Config m_config;
    int m_colorIndex;  // 使用颜色索引而非QColor对象，节省内存
    
    void generateColorIndex();
};

#endif // FOODBALL_H
