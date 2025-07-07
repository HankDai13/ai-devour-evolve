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

    // �����������ʱ�ķ��ѣ�ֻ��С����ʵ�ʷ�����Player/Controller����
    void splitRandom();

private:
    float x, y;
    float radius;
    QColor color;
    bool alive;
};

