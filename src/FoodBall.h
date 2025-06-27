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
    QColor m_color;
    
    void generateRandomColor();
};

#endif // FOODBALL_H
