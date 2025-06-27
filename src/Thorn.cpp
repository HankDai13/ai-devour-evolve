#include "Thorn.h"
#include "Ball.h"
#include <QPainter>
#include <cmath>

Thorn::Thorn(float x_, float y_, float r, QColor c)
    : x(x_), y(y_), radius(r), color(c), alive(true) {
}

void Thorn::render(QPainter* painter) {
    painter->setBrush(color);
    painter->setPen(Qt::black);
    // »æÖÆ´ø¾â³ÝµÄ¾£¼¬Çò
    int spikes = 12;
    QPolygonF poly;
    for (int i = 0; i < spikes * 2; ++i) {
        float angle = i * M_PI / spikes;
        float len = (i % 2 == 0) ? radius : radius * 0.7f;
        poly << QPointF(x + std::cos(angle) * len, y + std::sin(angle) * len);
    }
    painter->drawPolygon(poly);
    // ÄÚÔ²
    painter->setBrush(color.lighter(120));
    painter->drawEllipse(QPointF(x, y), radius * 0.7f, radius * 0.7f);
}

float Thorn::getX() const { return x; }
float Thorn::getY() const { return y; }
float Thorn::getRadius() const { return radius; }
bool Thorn::isAlive() const { return alive; }
void Thorn::kill() { alive = false; }

