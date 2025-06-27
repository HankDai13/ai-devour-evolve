#include "QuadTree.h"
#include "BaseBall.h"
#include <QDebug>
#include <algorithm>

QuadTree::QuadTree(const QRectF& bounds, int maxDepth, int maxBallsPerNode)
    : m_root(std::make_unique<Node>(bounds))
    , m_maxDepth(maxDepth)
    , m_maxBallsPerNode(maxBallsPerNode)
{
}

void QuadTree::insert(BaseBall* ball)
{
    if (!ball || ball->isRemoved()) {
        return;
    }
    
    insertNode(m_root.get(), ball, 0);
}

void QuadTree::insertNode(Node* node, BaseBall* ball, int depth)
{
    if (!node) return;
    
    QRectF ballBounds = getBallBounds(ball);
    
    // 检查球体是否在当前节点范围内
    if (!node->bounds.intersects(ballBounds)) {
        return;
    }
    
    // 如果是叶子节点且球数量未超限，直接添加
    if (node->isLeaf) {
        node->balls.append(ball);
        
        // 检查是否需要细分
        if (shouldSubdivide(node, depth)) {
            subdivide(node);
            
            // 重新分配球体到子节点
            QVector<BaseBall*> ballsToRedistribute = node->balls;
            node->balls.clear();
            
            for (BaseBall* b : ballsToRedistribute) {
                for (auto& child : node->children) {
                    insertNode(child.get(), b, depth + 1);
                }
            }
        }
    } else {
        // 不是叶子节点，插入到合适的子节点
        for (auto& child : node->children) {
            insertNode(child.get(), ball, depth + 1);
        }
    }
}

void QuadTree::subdivide(Node* node)
{
    if (!node || !node->isLeaf) return;
    
    qreal x = node->bounds.x();
    qreal y = node->bounds.y();
    qreal w = node->bounds.width() / 2.0;
    qreal h = node->bounds.height() / 2.0;
    
    // 创建四个子节点：西北、东北、西南、东南
    node->children[0] = std::make_unique<Node>(QRectF(x, y, w, h));         // NW
    node->children[1] = std::make_unique<Node>(QRectF(x + w, y, w, h));     // NE
    node->children[2] = std::make_unique<Node>(QRectF(x, y + h, w, h));     // SW
    node->children[3] = std::make_unique<Node>(QRectF(x + w, y + h, w, h)); // SE
    
    node->isLeaf = false;
}

bool QuadTree::shouldSubdivide(const Node* node, int depth) const
{
    return (node->balls.size() > m_maxBallsPerNode) && (depth < m_maxDepth);
}

QVector<BaseBall*> QuadTree::query(const QRectF& range) const
{
    QVector<BaseBall*> result;
    queryNode(m_root.get(), range, result);
    return result;
}

void QuadTree::queryNode(const Node* node, const QRectF& range, QVector<BaseBall*>& result) const
{
    if (!node || !node->bounds.intersects(range)) {
        return;
    }
    
    if (node->isLeaf) {
        // 叶子节点，检查每个球体
        for (BaseBall* ball : node->balls) {
            if (ball && !ball->isRemoved()) {
                QRectF ballBounds = getBallBounds(ball);
                if (range.intersects(ballBounds)) {
                    result.append(ball);
                }
            }
        }
    } else {
        // 非叶子节点，递归查询子节点
        for (const auto& child : node->children) {
            queryNode(child.get(), range, result);
        }
    }
}

QVector<BaseBall*> QuadTree::queryCollisions(BaseBall* ball) const
{
    if (!ball || ball->isRemoved()) {
        return QVector<BaseBall*>();
    }
    
    // 创建一个稍大的查询范围，考虑碰撞检测的误差
    QRectF ballBounds = getBallBounds(ball);
    qreal margin = ball->radius() * 0.1; // 10%的误差范围
    ballBounds.adjust(-margin, -margin, margin, margin);
    
    QVector<BaseBall*> candidates = query(ballBounds);
    
    // 移除自己
    candidates.removeAll(ball);
    
    return candidates;
}

void QuadTree::clear()
{
    if (m_root) {
        m_root->clear();
    }
}

void QuadTree::rebuild(const QVector<BaseBall*>& allBalls)
{
    clear();
    
    for (BaseBall* ball : allBalls) {
        insert(ball);
    }
}

QRectF QuadTree::getBallBounds(BaseBall* ball) const
{
    if (!ball) return QRectF();
    
    QPointF pos = ball->pos();
    qreal radius = ball->radius();
    
    return QRectF(pos.x() - radius, pos.y() - radius, 
                  radius * 2, radius * 2);
}

int QuadTree::getNodeCount() const
{
    return countNodes(m_root.get());
}

int QuadTree::countNodes(const Node* node) const
{
    if (!node) return 0;
    
    int count = 1; // 当前节点
    
    if (!node->isLeaf) {
        for (const auto& child : node->children) {
            count += countNodes(child.get());
        }
    }
    
    return count;
}

int QuadTree::getMaxDepth() const
{
    return getDepth(m_root.get());
}

int QuadTree::getDepth(const Node* node) const
{
    if (!node || node->isLeaf) return 1;
    
    int maxChildDepth = 0;
    for (const auto& child : node->children) {
        maxChildDepth = std::max(maxChildDepth, getDepth(child.get()));
    }
    
    return 1 + maxChildDepth;
}
