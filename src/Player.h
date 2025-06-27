#pragma once

#include <QColor>
#include <vector>
#include <memory>
#include <QPainter>
#include "HumanController.h"
#include "Ball.h"
#include "AIController.h"
class Ball;

class Player {
public:
    Player(int id, QColor color);
    Player(int id, QColor color, float x, float y, float radius); // 新增

    void update();
    void render(QPainter* painter);
    void moveTo(float x, float y);
    void setMoveDelta(float dx, float dy); // 用于持续移动（如方向键）
    void split(float targetX, float targetY);
    void ejectSpore();
    bool isAlive() const;
    int getId() const;
    std::vector<std::unique_ptr<Ball>>& getBalls();
    QColor getColor() const;
    void kill();

private:
    int id;
    QColor color;
    std::vector<std::unique_ptr<Ball>> balls;
    bool alive;
    float moveDeltaX = 0, moveDeltaY = 0;
};
