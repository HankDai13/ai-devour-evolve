#pragma once

#include <QColor>
#include <QPainter>

class PlayerBall {
public:
    PlayerBall(float x, float y, float radius, QColor color);

    void update();
    void render(QPainter* painter) const;

    float getX() const;
    float getY() const;
    float getRadius() const;
    QColor getColor() const;

    void setX(float x);
    void setY(float y);
    void setRadius(float r);
    void setColor(QColor c);

    void grow(float dr);
    void setAlive(bool alive);
    bool isAlive() const;
    void kill();

    float getVX() const;
    float getVY() const;
    void setVelocity(float vx, float vy);

private:
    float x, y;
    float radius;
    float vx, vy;
    QColor color;
    bool alive;
};