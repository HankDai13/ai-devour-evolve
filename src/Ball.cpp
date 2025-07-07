#include "Ball.h"
#include <QPainter>
#include <cmath>
#include <cstdlib>

Ball::Ball(float x_, float y_, float radius_, QColor color_)
    : x(x_), y(y_), radius(radius_), color(color_), alive(true)
{
}

void Ball::update() {
    // �������չ�ٶȡ�Ħ��������Ч��
}

void Ball::render(QPainter* painter) {
    painter->setBrush(color);
    painter->setPen(Qt::black);
    painter->drawEllipse(QPointF(x, y), radius, radius);
}

void Ball::moveTo(float nx, float ny) {
    x = nx;
    y = ny;
}

void Ball::grow(float delta) {
    radius += delta;
    if (radius < 6.0f) radius = 6.0f;
}

void Ball::setRadius(float r) {
    radius = r;
    if (radius < 6.0f) radius = 6.0f;
}

float Ball::getX() const { return x; }
float Ball::getY() const { return y; }
float Ball::getRadius() const { return radius; }
QColor Ball::getColor() const { return color; }
bool Ball::isAlive() const { return alive; }
void Ball::kill() { alive = false; }

void Ball::splitRandom() {
    // ֻ��С����ʵ��������Player/Controller����
    if (radius > 20.0f) {
        float r = radius / 1.414f;
        setRadius(r);
    }
}

