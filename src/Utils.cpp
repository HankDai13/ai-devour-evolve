#include "Utils.h"
#include <cstdlib>

float Utils::randomFloat(float min, float max) {
    return min + float(rand()) / float(RAND_MAX) * (max - min);
}

QColor Utils::randomColor() {
    int r = rand() % 200 + 30;
    int g = rand() % 200 + 30;
    int b = rand() % 200 + 30;
    return QColor(r, g, b);
}