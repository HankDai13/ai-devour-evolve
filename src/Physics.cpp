#include "Physics.h"
#include "Ball.h"
#include "Food.h"
#include "Thorn.h"
#include "Spore.h"
#include <cmath>

bool Physics::canEat(const Ball& eater, const Food& food) {
    float dx = eater.getX() - food.getX();
    float dy = eater.getY() - food.getY();
    float dist = std::sqrt(dx * dx + dy * dy);
    return dist < eater.getRadius();
}

bool Physics::canEat(const Ball& eater, const Spore& spore) {
    float dx = eater.getX() - spore.getX();
    float dy = eater.getY() - spore.getY();
    float dist = std::sqrt(dx * dx + dy * dy);
    return dist < eater.getRadius();
}

bool Physics::canEat(const Ball& eater, const Thorn& thorn, float ratio) {
    float dx = eater.getX() - thorn.getX();
    float dy = eater.getY() - thorn.getY();
    float dist = std::sqrt(dx * dx + dy * dy);
    return eater.getRadius() > thorn.getRadius() * ratio && dist < eater.getRadius();
}

bool Physics::canEat(const Ball& eater, const Ball& other, float ratio) {
    float dx = eater.getX() - other.getX();
    float dy = eater.getY() - other.getY();
    float dist = std::sqrt(dx * dx + dy * dy);
    return eater.getRadius() > other.getRadius() * ratio && dist < eater.getRadius();
}
