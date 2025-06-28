#ifndef COREUTILS_H
#define COREUTILS_H

#include <QVector>
#include <QRectF>
#include <QPointF>
#include <memory>
#include <array>

class BaseBallData;

// 用于核心逻辑的四叉树实现
class CoreQuadTree
{
public:
    struct Node {
        QRectF bounds;
        QVector<BaseBallData*> balls;
        std::array<std::unique_ptr<Node>, 4> children;
        bool isLeaf = true;
        
        Node(const QRectF& rect) : bounds(rect) {}
        
        void clear() {
            balls.clear();
            for (auto& child : children) {
                child.reset();
            }
            isLeaf = true;
        }
    };

    CoreQuadTree(const QRectF& bounds, int maxDepth = 6, int maxObjectsPerNode = 8);
    ~CoreQuadTree() = default;

    void rebuild(const QVector<BaseBallData*>& balls);
    QVector<BaseBallData*> queryCollisions(BaseBallData* ball) const;
    
    int getNodeCount() const;
    int getMaxDepth() const;

private:
    std::unique_ptr<Node> m_root;
    int m_maxDepth;
    int m_maxObjectsPerNode;
    
    void insert(Node* node, BaseBallData* ball, int depth);
    void subdivide(Node* node);
    void query(Node* node, BaseBallData* ball, QVector<BaseBallData*>& result) const;
    void countNodes(Node* node, int& count) const;
    void measureDepth(Node* node, int currentDepth, int& maxDepth) const;
};

#endif // COREUTILS_H
