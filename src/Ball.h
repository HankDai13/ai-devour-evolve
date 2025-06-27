#pragma once
#include <QColor>
#include <QPainter>
#include <cmath>

class Ball {
public:
    Ball(float x, float y, float radius, QColor color);

    void update();
    void render(QPainter* painter);

    void moveTo(float x, float y);
    void grow(float delta);
    void setRadius(float r);

    float getX() const;
    float getY() const;
    float getRadius() const;
    QColor getColor() const;

    bool isAlive() const;
    void kill();

    // 被荆棘球击中时的分裂（只缩小自身，实际分裂由Player/Controller负责）
    void splitRandom();

private:
    float x, y;
    float radius;
    QColor color;
    bool alive;
};

