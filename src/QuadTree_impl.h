#pragma once

#include <QRectF>
#include <cmath>

template<typename T>
QuadTree<T>::QuadTree(float x, float y, float w, float h, int depth, int maxDepth)
    : x(x), y(y), w(w), h(h), depth(depth), maxDepth(maxDepth), divided(false) {}

template<typename T>
void QuadTree<T>::insert(float px, float py, T* data) {
    if (depth == maxDepth) {
        objects.push_back(data);
        return;
    }
    if (!divided) subdivide();
    if (nw->contains(QRectF(nw->x, nw->y, nw->w, nw->h), px, py)) nw->insert(px, py, data);
    else if (ne->contains(QRectF(ne->x, ne->y, ne->w, ne->h), px, py)) ne->insert(px, py, data);
    else if (sw->contains(QRectF(sw->x, sw->y, sw->w, sw->h), px, py)) sw->insert(px, py, data);
    else if (se->contains(QRectF(se->x, se->y, se->w, se->h), px, py)) se->insert(px, py, data);
    else objects.push_back(data);
}

template<typename T>
void QuadTree<T>::queryRange(const QRectF& range, std::vector<T*>& results) {
    if (!QRectF(x, y, w, h).intersects(range)) return;
    for (auto* obj : objects) {
        if (contains(range, x, y)) results.push_back(obj);
    }
    if (divided) {
        nw->queryRange(range, results);
        ne->queryRange(range, results);
        sw->queryRange(range, results);
        se->queryRange(range, results);
    }
}

template<typename T>
void QuadTree<T>::subdivide() {
    float hw = w / 2, hh = h / 2;
    nw = std::make_unique<QuadTree>(x, y, hw, hh, depth + 1, maxDepth);
    ne = std::make_unique<QuadTree>(x + hw, y, hw, hh, depth + 1, maxDepth);
    sw = std::make_unique<QuadTree>(x, y + hh, hw, hh, depth + 1, maxDepth);
    se = std::make_unique<QuadTree>(x + hw, y + hh, hw, hh, depth + 1, maxDepth);
    divided = true;
}

template<typename T>
bool QuadTree<T>::contains(const QRectF& range, float px, float py) const {
    return range.contains(px, py);
}