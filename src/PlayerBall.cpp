#include "PlayerBall.h"
#include "HumanController.h"
#include "AIController.h"
PlayerBall::PlayerBall(float x, float y, float radius, QColor color)
    : x(x), y(y), radius(radius), vx(0), vy(0), color(color), alive(true) {}

void PlayerBall::update() {
    x += vx;
    y += vy;
    // 简单阻尼
    vx *= 0.95f;
    vy *= 0.95f;
}

void PlayerBall::render(QPainter* painter) const {
    painter->setBrush(QBrush(color));
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QPointF(x, y), radius, radius);
}

float PlayerBall::getX() const { return x; }
float PlayerBall::getY() const { return y; }
float PlayerBall::getRadius() const { return radius; }
QColor PlayerBall::getColor() const { return color; }

void PlayerBall::setX(float val) { x = val; }
void PlayerBall::setY(float val) { y = val; }
void PlayerBall::setRadius(float r) { radius = r; }
void PlayerBall::setColor(QColor c) { color = c; }

void PlayerBall::grow(float dr) { radius += dr; }
void PlayerBall::setAlive(bool a) { alive = a; }
bool PlayerBall::isAlive() const { return alive; }
void PlayerBall::kill() { alive = false; }

float PlayerBall::getVX() const { return vx; }
float PlayerBall::getVY() const { return vy; }
void PlayerBall::setVelocity(float vxx, float vyy) { vx = vxx; vy = vyy; }