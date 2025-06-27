#pragma once

#include <vector>
#include <memory>
#include <QRectF>

template<typename T>
class QuadTree {
public:
    QuadTree(float x, float y, float w, float h, int depth = 0, int maxDepth = 6);

    void insert(float x, float y, T* data);
    void queryRange(const QRectF& range, std::vector<T*>& results);

private:
    float x, y, w, h;
    int depth, maxDepth;
    std::vector<T*> objects;
    std::unique_ptr<QuadTree> nw, ne, sw, se;
    bool divided;

    void subdivide();
    bool contains(const QRectF& range, float px, float py) const;
};

#include "QuadTree_impl.h"