#include "Spore.h"
#include <cmath>

Spore::Spore(float x_, float y_, float r, QColor c)
    : x(x_), y(y_), radius(r), color(c), alive(true), vx(0), vy(0) {
}

void Spore::update() {
    x += vx;
    y += vy;
    vx *= 0.98f; // Ä¦²ÁÁ¦
    vy *= 0.98f;
    if (std::abs(vx) < 0.1f && std::abs(vy) < 0.1f) { vx = 0; vy = 0; }
}

void Spore::render(QPainter* painter) {
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QPointF(x, y), radius, radius);
}

float Spore::getX() const { return x; }
float Spore::getY() const { return y; }
float Spore::getRadius() const { return radius; }
bool Spore::isAlive() const { return alive; }
void Spore::kill() { alive = false; }
