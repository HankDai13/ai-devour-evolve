#ifndef QUADTREE_H
#define QUADTREE_H

#include <QVector>
#include <QRectF>
#include <QPointF>
#include <memory>
#include <array>

class BaseBall;

class QuadTree
{
public:
    struct Node {
        QRectF bounds;
        QVector<BaseBall*> balls;
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

    QuadTree(const QRectF& bounds, int maxDepth = 6, int maxBallsPerNode = 8);
    ~QuadTree() = default;

    // 插入球体
    void insert(BaseBall* ball);
    
    // 查询指定区域内的球体
    QVector<BaseBall*> query(const QRectF& range) const;
    
    // 查询与指定球体可能碰撞的球体
    QVector<BaseBall*> queryCollisions(BaseBall* ball) const;
    
    // 清空四叉树
    void clear();
    
    // 重建四叉树（用于每帧更新）
    void rebuild(const QVector<BaseBall*>& allBalls);
    
    // 获取统计信息
    int getNodeCount() const;
    int getMaxDepth() const;

private:
    std::unique_ptr<Node> m_root;
    int m_maxDepth;
    int m_maxBallsPerNode;
    
    void insertNode(Node* node, BaseBall* ball, int depth);
    void queryNode(const Node* node, const QRectF& range, QVector<BaseBall*>& result) const;
    void subdivide(Node* node);
    bool shouldSubdivide(const Node* node, int depth) const;
    QRectF getBallBounds(BaseBall* ball) const;
    int countNodes(const Node* node) const;
    int getDepth(const Node* node) const;
};

#endif // QUADTREE_H
