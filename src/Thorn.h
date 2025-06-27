#pragma once

#include <QColor>
#include <QPainter>

class Thorn {
public:
    Thorn(float x, float y, float radius, QColor color);

    void render(QPainter* painter);
    float getX() const;
    float getY() const;
    float getRadius() const;
    bool isAlive() const;
    void kill();

private:
    float x, y, radius;
    QColor color;
    bool alive;
};