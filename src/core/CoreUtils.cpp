#include "CoreUtils.h"
#include "data/BaseBallData.h"

CoreQuadTree::CoreQuadTree(const QRectF& bounds, int maxDepth, int maxObjectsPerNode)
    : m_root(std::make_unique<Node>(bounds))
    , m_maxDepth(maxDepth)
    , m_maxObjectsPerNode(maxObjectsPerNode)
{
}

void CoreQuadTree::rebuild(const QVector<BaseBallData*>& balls)
{
    m_root->clear();
    
    for (BaseBallData* ball : balls) {
        if (ball && !ball->isRemoved()) {
            insert(m_root.get(), ball, 0);
        }
    }
}

QVector<BaseBallData*> CoreQuadTree::queryCollisions(BaseBallData* ball) const
{
    QVector<BaseBallData*> result;
    if (!ball || ball->isRemoved()) {
        return result;
    }
    
    query(m_root.get(), ball, result);
    return result;
}

int CoreQuadTree::getNodeCount() const
{
    int count = 0;
    countNodes(m_root.get(), count);
    return count;
}

int CoreQuadTree::getMaxDepth() const
{
    int maxDepth = 0;
    measureDepth(m_root.get(), 0, maxDepth);
    return maxDepth;
}

void CoreQuadTree::insert(Node* node, BaseBallData* ball, int depth)
{
    if (!node || !ball) return;
    
    // 检查球是否在节点边界内
    QPointF pos = ball->position();
    if (!node->bounds.contains(pos)) {
        return;
    }
    
    // 如果是叶子节点且未达到分割条件，直接添加
    if (node->isLeaf) {
        node->balls.append(ball);
        
        // 检查是否需要分割
        if (node->balls.size() > m_maxObjectsPerNode && depth < m_maxDepth) {
            subdivide(node);
            
            // 重新分配球到子节点
            QVector<BaseBallData*> ballsToRedistribute = node->balls;
            node->balls.clear();
            
            for (BaseBallData* b : ballsToRedistribute) {
                for (auto& child : node->children) {
                    if (child) {
                        insert(child.get(), b, depth + 1);
                    }
                }
            }
        }
    } else {
        // 非叶子节点，递归插入到子节点
        for (auto& child : node->children) {
            if (child) {
                insert(child.get(), ball, depth + 1);
            }
        }
    }
}

void CoreQuadTree::subdivide(Node* node)
{
    if (!node) return;
    
    qreal x = node->bounds.x();
    qreal y = node->bounds.y();
    qreal width = node->bounds.width();
    qreal height = node->bounds.height();
    qreal halfWidth = width / 2;
    qreal halfHeight = height / 2;
    
    // 创建四个子节点
    node->children[0] = std::make_unique<Node>(QRectF(x, y, halfWidth, halfHeight)); // 左上
    node->children[1] = std::make_unique<Node>(QRectF(x + halfWidth, y, halfWidth, halfHeight)); // 右上
    node->children[2] = std::make_unique<Node>(QRectF(x, y + halfHeight, halfWidth, halfHeight)); // 左下
    node->children[3] = std::make_unique<Node>(QRectF(x + halfWidth, y + halfHeight, halfWidth, halfHeight)); // 右下
    
    node->isLeaf = false;
}

void CoreQuadTree::query(Node* node, BaseBallData* ball, QVector<BaseBallData*>& result) const
{
    if (!node || !ball) return;
    
    // 检查查询范围是否与节点边界相交
    QPointF pos = ball->position();
    qreal radius = ball->radius();
    QRectF queryRect(pos.x() - radius, pos.y() - radius, radius * 2, radius * 2);
    
    if (!node->bounds.intersects(queryRect)) {
        return;
    }
    
    // 如果是叶子节点，添加所有球
    if (node->isLeaf) {
        for (BaseBallData* b : node->balls) {
            if (b && !b->isRemoved() && b != ball) {
                result.append(b);
            }
        }
    } else {
        // 递归查询子节点
        for (const auto& child : node->children) {
            if (child) {
                query(child.get(), ball, result);
            }
        }
    }
}

void CoreQuadTree::countNodes(Node* node, int& count) const
{
    if (!node) return;
    
    count++;
    for (const auto& child : node->children) {
        if (child) {
            countNodes(child.get(), count);
        }
    }
}

void CoreQuadTree::measureDepth(Node* node, int currentDepth, int& maxDepth) const
{
    if (!node) return;
    
    maxDepth = qMax(maxDepth, currentDepth);
    
    for (const auto& child : node->children) {
        if (child) {
            measureDepth(child.get(), currentDepth + 1, maxDepth);
        }
    }
}
