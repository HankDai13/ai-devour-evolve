#include "GameView.h"
#include "GameManager.h"
#include "CloneBall.h"
#include "FoodBall.h"
#include "SporeBall.h"
#include "BaseBall.h"
#include "GoBiggerConfig.h"
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QSet>
#include <QColor>
#include <QVector2D>
#include <QDebug>
#include <QCursor>
#include <QPainter>
#include <QLineF>
#include <cmath>

GameView::GameView(QWidget *parent) 
    : QGraphicsView(parent)
    , m_gameManager(nullptr)
    , m_mainPlayer(nullptr)
    , m_inputTimer(nullptr)
    , m_zoomFactor(1.0)
    , m_followPlayer(true)
    , m_targetZoom(1.0)
    , m_minVisionRadius(400.0)     // 进一步增大最小视野半径，开局更大视野
    , m_maxVisionRadius(800.0)     // 增大最大视野范围
    , m_scaleUpRatio(2.5)          // 稍微增加缩放比例，但不要太快
{
    initializeView();
    initializePlayer();
    setupConnections();
}

GameView::~GameView()
{
    if (m_inputTimer) {
        m_inputTimer->stop();
        delete m_inputTimer;
    }
    
    if (m_gameManager) {
        delete m_gameManager;
    }
}

void GameView::initializeView()
{
    // 创建场景 - 扩大到与GoBiggerConfig::MAP_WIDTH/HEIGHT一致
    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->setSceneRect(-3000, -3000, 6000, 6000); // 6000x6000的游戏世界
    scene->setBackgroundBrush(QColor(240, 245, 250)); // 浅蓝灰色背景
    
    // 设置视图 - 简化版本，避免复杂优化
    setScene(scene);
    
    // 基础渲染设置
    setRenderHint(QPainter::Antialiasing, false);  // 关闭抗锯齿
    setCacheMode(QGraphicsView::CacheBackground);   // 只缓存背景
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);  // 全视口更新，简单可靠
    setOptimizationFlags(QGraphicsView::DontSavePainterState); // 基础优化
    
    // 禁用滚动条，使用自定义相机控制
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragMode(QGraphicsView::NoDrag);
    
    // 启用键盘焦点
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    
    // 创建游戏管理器 - 使用扩大的地图边界
    GameManager::Config config;
    config.gameBorder = Border(-3000, 3000, -3000, 3000); // 6000x6000的游戏边界
    m_gameManager = new GameManager(scene, config, this);
    
    // 创建输入处理定时器
    m_inputTimer = new QTimer(this);
    connect(m_inputTimer, &QTimer::timeout, this, &GameView::updateGameView);
    m_inputTimer->start(16); // 60 FPS
}

void GameView::initializePlayer()
{
    // 先启动游戏
    if (m_gameManager) {
        m_gameManager->startGame();
        qDebug() << "Game started";
    }
    
    // 创建主玩家
    m_mainPlayer = m_gameManager->createPlayer(0, 0, QPointF(0, 0)); // 团队0，玩家0，中心位置
    
    if (m_mainPlayer) {
        // 设置一个合理的初始分数，让玩家球更大一些
        m_mainPlayer->setScore(GoBiggerConfig::CELL_INIT_SCORE); // 使用新的标准初始分数
        
        // 立即将视图中心设置到玩家位置
        centerOn(m_mainPlayer->pos());
        
        qDebug() << "Main player created with ID:" << m_mainPlayer->ballId() 
                 << "at position:" << m_mainPlayer->pos()
                 << "with radius:" << m_mainPlayer->radius()
                 << "with score:" << m_mainPlayer->score();
    } else {
        qDebug() << "Failed to create main player!";
    }
}

void GameView::setupConnections()
{
    if (m_gameManager) {
        connect(m_gameManager, &GameManager::gameStarted, this, &GameView::onGameStarted);
        connect(m_gameManager, &GameManager::gamePaused, this, &GameView::onGamePaused);
        connect(m_gameManager, &GameManager::gameReset, this, &GameView::onGameReset);
        connect(m_gameManager, &GameManager::playerAdded, this, &GameView::onPlayerAdded);
        connect(m_gameManager, &GameManager::playerRemoved, this, &GameView::onPlayerRemoved);
    }
}

void GameView::startGame()
{
    if (m_gameManager) {
        m_gameManager->startGame();
    }
}

void GameView::pauseGame()
{
    if (m_gameManager) {
        m_gameManager->pauseGame();
    }
}

void GameView::resetGame()
{
    if (m_gameManager) {
        m_gameManager->resetGame();
        // 重新创建主玩家
        initializePlayer();
    }
}

bool GameView::isGameRunning() const
{
    return m_gameManager ? m_gameManager->isGameRunning() : false;
}

void GameView::keyPressEvent(QKeyEvent *event)
{
    m_pressedKeys.insert(event->key());
    
    qDebug() << "Key pressed:" << event->key() << "Text:" << event->text();
    
    // GoBigger风格按键映射
    switch (event->key()) {
        case Qt::Key_W:
            qDebug() << "W key detected - calling handleSplitAction";
            handleSplitAction();
            break;
        case Qt::Key_Q:
            qDebug() << "Q key detected - calling handleEjectAction";
            handleEjectAction();
            break;
        case Qt::Key_P:
            if (isGameRunning()) {
                pauseGame();
            } else {
                startGame();
            }
            break;
        case Qt::Key_Escape:
            resetGame();
            break;
        // 移除WASD移动控制，改为鼠标控制
    }
    
    QGraphicsView::keyPressEvent(event);
}

void GameView::keyReleaseEvent(QKeyEvent *event)
{
    m_pressedKeys.remove(event->key());
    QGraphicsView::keyReleaseEvent(event);
}

void GameView::mousePressEvent(QMouseEvent *event)
{
    // GoBigger风格：移除鼠标点击分裂/喷射，专注于鼠标位置控制移动
    QGraphicsView::mousePressEvent(event);
}

void GameView::wheelEvent(QWheelEvent *event)
{
    // 缩放控制 - 降低缩放速度，更加平滑
    const qreal scaleFactor = 1.08; // 从1.15降低到1.08，缩放更平滑
    
    if (event->angleDelta().y() > 0) {
        // 放大
        m_zoomFactor *= scaleFactor;
        scale(scaleFactor, scaleFactor);
    } else {
        // 缩小
        m_zoomFactor /= scaleFactor;
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
    
    // 限制缩放范围 - 更合理的缩放上下限
    m_zoomFactor = qBound(0.3, m_zoomFactor, 2.5);
}

void GameView::updateGameView()
{
    processInput();
    updateCamera();
    // 移除视野裁剪更新，恢复简单模式
}

void GameView::processInput()
{
    if (!m_mainPlayer || m_mainPlayer->isRemoved()) {
        return;
    }

    // GoBigger风格：获取所有玩家球（包括分裂球）
    QVector<CloneBall*> allPlayerBalls = getAllPlayerBalls();
    
    if (allPlayerBalls.isEmpty()) {
        return;
    }

    // 计算质心位置（用于向心力计算）
    QPointF centroid = calculatePlayerCentroidAll(allPlayerBalls);
    
    // 获取鼠标在场景中的位置
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    QPointF sceneMousePos = mapToScene(mousePos);
    
    // 对每个球应用GoBigger的独立控制机制
    for (CloneBall* ball : allPlayerBalls) {
        if (!ball || ball->isRemoved()) continue;
        
        // 1. 每个球独立计算到鼠标的方向（关键修复！）
        QPointF ballPos = ball->pos();
        QVector2D toMouse(sceneMousePos.x() - ballPos.x(), sceneMousePos.y() - ballPos.y());
        
        // 只有当鼠标距离足够远时才移动（避免在球中心时抖动）
        float minDistance = 15.0f;
        QVector2D playerInput(0, 0);
        if (toMouse.length() > minDistance) {
            playerInput = toMouse.normalized();
        }
        
        // 2. 向心力加速度 (given_acc_center) - 只在有多个球时生效
        QVector2D centerForce(0, 0);
        if (allPlayerBalls.size() > 1) {
            QVector2D toCenter(centroid.x() - ballPos.x(), centroid.y() - ballPos.y());
            float distanceToCenter = toCenter.length();
            
            // 更温和的向心力机制：只有距离足够远且在合理范围内才应用
            float minDistance = ball->radius() * 2.0f; // 最小距离：2倍半径
            float maxDistance = ball->radius() * 8.0f; // 最大距离：8倍半径
            
            if (distanceToCenter > minDistance && distanceToCenter < maxDistance) {
                // 基于距离的线性衰减向心力，距离越远力越小
                float forceRatio = 1.0f - (distanceToCenter - minDistance) / (maxDistance - minDistance);
                float forceStrength = 0.3f * forceRatio; // 减小基础力度
                
                centerForce = toCenter.normalized() * forceStrength;
            }
        }
        
        // 应用GoBigger风格的移动控制
        ball->applyGoBiggerMovement(playerInput, centerForce);
        
        // 更新球的移动方向（用于箭头显示）
        ball->setMoveDirection(playerInput);
    }
}

QVector2D GameView::calculateMouseDirection() const
{
    // 获取鼠标在视图中的位置
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    
    // 转换为场景坐标
    QPointF sceneMousePos = mapToScene(mousePos);
    
    // 计算玩家质心位置
    QPointF playerCentroid = calculatePlayerCentroid();
    
    // 计算从玩家到鼠标的方向向量
    QVector2D direction(sceneMousePos.x() - playerCentroid.x(), 
                       sceneMousePos.y() - playerCentroid.y());
    
    // 只有当鼠标距离足够远时才移动（避免鼠标在球中心时抖动）
    float minDistance = 10.0f;
    if (direction.length() < minDistance) {
        return QVector2D(0, 0);
    }
    
    return direction.normalized();
}

QVector2D GameView::calculateMoveDirection() const
{
    // 保留旧的WASD方法作为备用（可能在调试时有用）
    QVector2D direction(0, 0);
    
    // 移除WASD移动，现在仅用于其他功能
    // if (m_pressedKeys.contains(Qt::Key_W) || m_pressedKeys.contains(Qt::Key_Up)) {
    //     direction.setY(direction.y() - 1);
    // }
    
    return direction.length() > 0 ? direction.normalized() : direction;
}

void GameView::updateCamera()
{
    if (!m_followPlayer || !m_mainPlayer || m_mainPlayer->isRemoved()) {
        return;
    }
    
    // 计算所有玩家球的质心位置
    QVector<CloneBall*> allPlayerBalls = getAllPlayerBalls();
    QPointF centroid = calculatePlayerCentroidAll(allPlayerBalls);
    centerOn(centroid);
    
    // 计算智能缩放（基于GoBigger原版算法）
    calculateIntelligentZoomGoBigger(allPlayerBalls);
    
    // 应用平滑缩放
    adjustZoom();
}

void GameView::calculateIntelligentZoomGoBigger(const QVector<CloneBall*>& allPlayerBalls)
{
    if (allPlayerBalls.isEmpty()) {
        return;
    }
    
    // GoBigger风格视野计算：
    // 1. 计算所有球的最小外接矩形
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    
    float maxRadius = 0;
    
    for (CloneBall* ball : allPlayerBalls) {
        if (!ball || ball->isRemoved()) continue;
        
        QPointF pos = ball->pos();
        float radius = ball->radius();
        
        minX = std::min(minX, static_cast<float>(pos.x() - radius));
        maxX = std::max(maxX, static_cast<float>(pos.x() + radius));
        minY = std::min(minY, static_cast<float>(pos.y() - radius));
        maxY = std::max(maxY, static_cast<float>(pos.y() + radius));
        
        maxRadius = std::max(maxRadius, radius);
    }
    
    // 2. 计算外接矩形的大小
    float rectWidth = maxX - minX;
    float rectHeight = maxY - minY;
    float maxDimension = std::max(rectWidth, rectHeight);
    
    // 3. 应用最小视野保证（减小倍数，避免视角过大）
    float minVisionSize = maxRadius * 8.0f; // 从12倍减少到8倍最大球半径的视野
    float requiredVisionSize = std::max(maxDimension, minVisionSize);
    
    // 4. 应用合理的放大系数（减小放大倍数）
    requiredVisionSize *= m_scaleUpRatio; // 现在是2.2倍放大（之前是3.5倍）
    
    // 5. 计算目标缩放比例
    float viewportSize = std::min(width(), height()) * 0.75f; // 从80%调整到75%
    m_targetZoom = viewportSize / requiredVisionSize;
    
    // 6. 更严格的缩放范围限制
    m_targetZoom = qBound(0.4, m_targetZoom, 1.8); // 缩小上限从2.0到1.8
    
    qDebug() << "Vision calculation - rectSize:" << maxDimension 
             << "maxRadius:" << maxRadius 
             << "requiredVision:" << requiredVisionSize 
             << "targetZoom:" << m_targetZoom;
}

qreal GameView::calculatePlayerRadius() const
{
    if (!m_mainPlayer || m_mainPlayer->isRemoved()) {
        return m_minVisionRadius;
    }
    
    // 如果玩家只有一个球，直接返回球的半径
    // 这里简化处理，实际GoBigger会考虑所有分裂的球
    QPointF centroid = m_mainPlayer->pos();
    qreal playerRadius = m_mainPlayer->radius();
    
    // 模拟GoBigger的计算方式：
    // 找到距离质心最远的球的边界作为视野半径
    qreal maxDistance = playerRadius; // 至少包含当前球的半径
    
    // 如果有多个分裂球，需要计算更大的视野
    // 这里简化为根据球的大小动态调整
    maxDistance = qMax(maxDistance, playerRadius * 1.5);
    
    return maxDistance;
}

QPointF GameView::calculatePlayerCentroid() const
{
    if (!m_mainPlayer || m_mainPlayer->isRemoved()) {
        return QPointF(0, 0);
    }
    
    // 简化版本：直接返回主球位置
    // 实际GoBigger会计算所有分裂球的质心
    return m_mainPlayer->pos();
}

void GameView::adjustZoom()
{
    // 平滑缩放过渡
    qreal currentZoom = transform().m11();
    qreal zoomDiff = m_targetZoom - currentZoom;
    
    if (std::abs(zoomDiff) > 0.001) {
        // 使用更平滑的过渡速度
        qreal transitionSpeed = 0.08; // 8%的过渡速度，比之前更快
        qreal newZoom = currentZoom + zoomDiff * transitionSpeed;
        
        // 重置变换并应用新缩放
        resetTransform();
        scale(newZoom, newZoom);
        m_zoomFactor = newZoom;
    }
}

void GameView::handleSplitAction()
{
    if (!m_mainPlayer || m_mainPlayer->isRemoved()) {
        return;
    }
    
    // GoBigger风格：所有玩家球都能分裂
    QVector<CloneBall*> allPlayerBalls = getAllPlayerBalls();
    
    // 获取鼠标位置用于分裂方向
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    QPointF sceneMousePos = mapToScene(mousePos);
    
    QVector<CloneBall*> allNewBalls;
    
    for (CloneBall* ball : allPlayerBalls) {
        if (!ball || ball->isRemoved()) continue;
        
        if (ball->canSplit()) {
            // 每个球独立计算到鼠标的分裂方向
            QPointF ballPos = ball->pos();
            QVector2D toMouse(sceneMousePos.x() - ballPos.x(), sceneMousePos.y() - ballPos.y());
            
            QVector2D splitDirection;
            if (toMouse.length() > 10.0f) {
                splitDirection = toMouse.normalized();
            } else {
                splitDirection = QVector2D(1, 0); // 默认向右分裂
            }
            
            QVector<CloneBall*> newBalls = ball->performSplit(splitDirection);
            allNewBalls.append(newBalls);
            
            qDebug() << "Ball" << ball->ballId() << "split, created" << newBalls.size() << "new balls";
        }
    }
    
    if (!allNewBalls.isEmpty()) {
        qDebug() << "Total split performed, created" << allNewBalls.size() << "new balls";
    } else {
        qDebug() << "No balls could split: insufficient score or cooldown";
    }
}

void GameView::handleEjectAction()
{
    qDebug() << "handleEjectAction called";
    
    if (!m_mainPlayer || m_mainPlayer->isRemoved()) {
        qDebug() << "No main player or player removed";
        return;
    }
    
    // GoBigger风格：所有玩家球都能吐孢子
    QVector<CloneBall*> allPlayerBalls = getAllPlayerBalls();
    
    // 获取鼠标位置用于孢子方向
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    QPointF sceneMousePos = mapToScene(mousePos);
    
    QVector<SporeBall*> allSpores;
    
    for (CloneBall* ball : allPlayerBalls) {
        if (!ball || ball->isRemoved()) continue;
        
        qDebug() << "Ball" << ball->ballId() << "canEject:" << ball->canEject() 
                 << "score:" << ball->score();
        
        if (ball->canEject()) {
            // 每个球独立计算到鼠标的喷射方向
            QPointF ballPos = ball->pos();
            QVector2D toMouse(sceneMousePos.x() - ballPos.x(), sceneMousePos.y() - ballPos.y());
            
            QVector2D ejectDirection;
            if (toMouse.length() > 10.0f) {
                ejectDirection = toMouse.normalized();
            } else {
                ejectDirection = QVector2D(1, 0); // 默认向右喷射
            }
            
            qDebug() << "Ball" << ball->ballId() << "ejecting spore in direction:" 
                     << ejectDirection.x() << ejectDirection.y();
            
            auto* spore = ball->ejectSpore(ejectDirection);
            if (spore) {
                allSpores.append(spore);
                qDebug() << "Ball" << ball->ballId() << "ejected spore successfully at position:" 
                         << spore->pos().x() << spore->pos().y()
                         << "with velocity:" << spore->velocity().length();
            } else {
                qDebug() << "Ball" << ball->ballId() << "failed to create spore";
            }
        }
    }
    
    if (!allSpores.isEmpty()) {
        qDebug() << "Total spores ejected:" << allSpores.size();
    } else {
        qDebug() << "No balls could eject: insufficient score";
    }
}

void GameView::onGameStarted()
{
    qDebug() << "Game started - View updated";
}

void GameView::onGamePaused()
{
    qDebug() << "Game paused - View updated";
}

void GameView::onGameReset()
{
    qDebug() << "Game reset - View updated";
    // 重置相机
    resetTransform();
    m_zoomFactor = 1.0;
}

void GameView::onPlayerAdded(CloneBall* player)
{
    qDebug() << "Player added to view:" << player->ballId();
}

void GameView::onPlayerRemoved(CloneBall* player)
{
    qDebug() << "Player removed from view:" << player->ballId();
    
    // 如果被移除的是主玩家，寻找替代
    if (player == m_mainPlayer) {
        m_mainPlayer = nullptr;
        
        // 寻找同队的其他球作为新的主球
        QVector<CloneBall*> players = m_gameManager->getPlayers();
        for (CloneBall* p : players) {
            if (p && !p->isRemoved() && p->teamId() == 0) {
                m_mainPlayer = p;
                qDebug() << "New main player:" << m_mainPlayer->ballId();
                break;
            }
        }
    }
}

QVector<CloneBall*> GameView::getAllPlayerBalls() const
{
    QVector<CloneBall*> allBalls;
    
    if (!m_gameManager) {
        return allBalls;
    }
    
    // 获取游戏管理器中的所有玩家球
    QVector<CloneBall*> players = m_gameManager->getPlayers();
    for (CloneBall* player : players) {
        if (player && !player->isRemoved() && player->teamId() == 0) { // 只获取team 0的球
            allBalls.append(player);
        }
    }
    
    return allBalls;
}

QPointF GameView::calculatePlayerCentroidAll(const QVector<CloneBall*>& balls) const
{
    if (balls.isEmpty()) {
        return QPointF(0, 0);
    }
    
    // GoBigger风格：根据质量计算加权质心
    float totalScore = 0;
    QPointF weightedSum(0, 0);
    
    for (CloneBall* ball : balls) {
        if (ball && !ball->isRemoved()) {
            float score = ball->score();
            QPointF pos = ball->pos();
            weightedSum += pos * score;
            totalScore += score;
        }
    }
    
    if (totalScore > 0) {
        return weightedSum / totalScore;
    } else {
        return balls.first()->pos(); // 备选方案
    }
}

// ============ 视野裁剪优化实现（已禁用以提升性能）============

/*
QRectF GameView::getVisibleWorldRect() const
{
    // 获取当前视图在场景中的可见矩形区域
    QRect viewRect = viewport()->rect();
    QPolygonF scenePolygon = mapToScene(viewRect);
    QRectF sceneRect = scenePolygon.boundingRect();
    
    // 扩大一点边界，确保边缘对象正确处理
    qreal margin = 200.0;
    sceneRect.adjust(-margin, -margin, margin, margin);
    
    return sceneRect;
}

void GameView::updateVisibleItems()
{
    // 功能已禁用，避免额外的性能开销
    return;
}
*/

// ============ 渲染优化实现（简化版）============

void GameView::drawBackground(QPainter *painter, const QRectF &rect)
{
    // 简单绘制背景，移除额外的剪裁处理
    QGraphicsView::drawBackground(painter, rect);
}