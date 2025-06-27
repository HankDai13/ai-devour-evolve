#include "Food.h"

Food::Food(float x_, float y_, float r, QColor c)
    : x(x_), y(y_), radius(r), color(c), alive(true) {
}

void Food::render(QPainter* painter) {
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QPointF(x, y), radius, radius);
}

float Food::getX() const { return x; }
float Food::getY() const { return y; }
float Food::getRadius() const { return radius; }
bool Food::isAlive() const { return alive; }
void Food::kill() { alive = false; }