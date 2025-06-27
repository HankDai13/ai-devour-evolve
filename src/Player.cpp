#include "Player.h"
#include "Ball.h"
#include <QPainter>
#include <cmath>

Player::Player(int id_, QColor color_)
    : id(id_), color(color_), alive(true)
{
    balls.emplace_back(std::make_unique<Ball>(400, 300, 24.0f, color));
}

Player::Player(int id_, QColor color_, float x, float y, float radius)
    : id(id_), color(color_), alive(true)
{
    balls.emplace_back(std::make_unique<Ball>(x, y, radius, color));
}

void Player::update() {
    for (auto& ball : balls) {
        if (ball->isAlive()) ball->update();
    }
    // WASD/方向键持续移动
    if ((moveDeltaX != 0 || moveDeltaY != 0) && !balls.empty()) {
        float px = balls[0]->getX();
        float py = balls[0]->getY();
        float speed = 5.0f;
        moveTo(px + moveDeltaX * speed, py + moveDeltaY * speed);
    }
    // 检查所有球都死亡则玩家死亡
    bool anyAlive = false;
    for (const auto& ball : balls) {
        if (ball->isAlive()) { anyAlive = true; break; }
    }
    alive = anyAlive;
}

void Player::render(QPainter* painter) {
    for (const auto& ball : balls)
        if (ball->isAlive()) ball->render(painter);
}

void Player::moveTo(float x, float y) {
    for (auto& ball : balls)
        if (ball->isAlive()) ball->moveTo(x, y);
}

void Player::setMoveDelta(float dx, float dy) {
    moveDeltaX = dx;
    moveDeltaY = dy;
}

void Player::split(float targetX, float targetY) {
    Ball* maxBall = nullptr;
    float maxR = 0;
    for (auto& ball : balls) {
        if (!ball->isAlive()) continue;
        if (ball->getRadius() > maxR) {
            maxR = ball->getRadius();
            maxBall = ball.get();
        }
    }
    if (maxBall && maxBall->getRadius() > 20.0f) {
        float r = maxBall->getRadius() / 1.414f;
        float angle = std::atan2(targetY - maxBall->getY(), targetX - maxBall->getX());
        float nx = maxBall->getX() + std::cos(angle) * r * 2;
        float ny = maxBall->getY() + std::sin(angle) * r * 2;

        balls.emplace_back(std::make_unique<Ball>(nx, ny, r, color));
        maxBall->setRadius(r);
    }
}

void Player::ejectSpore() {
    Ball* maxBall = nullptr;
    float maxR = 0;
    for (auto& ball : balls) {
        if (!ball->isAlive()) continue;
        if (ball->getRadius() > maxR) {
            maxR = ball->getRadius();
            maxBall = ball.get();
        }
    }
    if (maxBall && maxBall->getRadius() > 18.0f) {
        maxBall->setRadius(maxBall->getRadius() - 2.0f);
        // 可扩展：通知GameController生成孢子
    }
}

bool Player::isAlive() const { return alive; }
int Player::getId() const { return id; }
std::vector<std::unique_ptr<Ball>>& Player::getBalls() { return balls; }
QColor Player::getColor() const { return color; }
void Player::kill() { alive = false; }
