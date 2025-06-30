#include "GameView.h"
#include "GameManager.h"
#include "CloneBall.h"
#include "FoodBall.h"
#include "SporeBall.h"
#include "BaseBall.h"
#include "SimpleAIPlayer.h"
#include "AIDebugWidget.h"
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
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineF>
#include <QMessageBox>
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
    , m_maxVisionRadius(600.0)     // 增大最大视野范围
    , m_scaleUpRatio(1.8)          // 稍微增加缩放比例，但不要太快
    , m_aiDebugWidget(nullptr)
{
    initializeView();
    initializePlayer();
    setupConnections();
    
    // 初始化AI调试窗口
    m_aiDebugWidget = new AIDebugWidget(nullptr, this);
    m_aiDebugWidget->hide(); // 默认隐藏
}

GameView::~GameView()
{
    if (m_inputTimer) {
        m_inputTimer->stop();
        delete m_inputTimer;
    }
    
    if (m_aiDebugWidget) {
        delete m_aiDebugWidget;
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
    
    // 更新AI调试窗口的GameManager引用
    if (m_aiDebugWidget) {
        delete m_aiDebugWidget;
        m_aiDebugWidget = new AIDebugWidget(m_gameManager, this);
        m_aiDebugWidget->hide();
    }
    
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

void GameView::onAIPlayerDestroyed(GoBigger::AI::SimpleAIPlayer* aiPlayer)
{
    if (!aiPlayer) return;
    
    qDebug() << "GameView: AI player destroyed, removing from debug widget";
    
    // 从调试台中移除AI
    if (m_aiDebugWidget) {
        m_aiDebugWidget->removeAIPlayer(aiPlayer);
    }
    
    // 从本地AI列表中移除（如果有的话）
    m_aiPlayers.removeOne(aiPlayer);
    
    qDebug() << "GameView: AI player cleanup completed";
}

QVector<CloneBall*> GameView::getAllPlayerBalls() const
{
    QVector<CloneBall*> allBalls;
    
    if (!m_gameManager || !m_mainPlayer) {
        return allBalls;
    }
    
    // 只获取主玩家的球（teamId=0, playerId=0），不包括AI球
    QVector<CloneBall*> players = m_gameManager->getPlayers();
    for (CloneBall* player : players) {
        if (player && !player->isRemoved() && 
            player->teamId() == 0 && player->playerId() == 0) {
            allBalls.append(player);
        }
    }
    
    return allBalls;
}

// AI调试功能实现
void GameView::showAIDebugConsole()
{
    if (m_aiDebugWidget) {
        if (m_gameManager) {
            m_aiDebugWidget->setParent(nullptr); // 独立窗口
            m_aiDebugWidget->show();
            m_aiDebugWidget->raise();
            m_aiDebugWidget->activateWindow();
        }
    }
}

void GameView::toggleAIDebugConsole()
{
    if (m_aiDebugWidget) {
        if (m_aiDebugWidget->isVisible()) {
            m_aiDebugWidget->hide();
        } else {
            showAIDebugConsole();
        }
    }
}

// 重写AI控制函数以集成调试功能
void GameView::addAIPlayer()
{
    if (!m_gameManager) return;
    
    static int aiPlayerCount = 1;
    
    // 使用GameManager添加AI玩家 - 使用teamId=1区分于主玩家(teamId=0), 默认使用FOOD_HUNTER策略
    bool success = m_gameManager->addAIPlayerWithStrategy(1, aiPlayerCount++, 
                                                         GoBigger::AI::AIStrategy::FOOD_HUNTER, "");
    
    if (success && m_aiDebugWidget) {
        // 从GameManager获取最新添加的AI
        auto aiPlayers = m_gameManager->getAIPlayers();
        if (!aiPlayers.isEmpty()) {
            auto lastAI = aiPlayers.last();
            m_aiDebugWidget->addAIPlayer(lastAI);
            
            // 连接AI销毁信号
            connect(lastAI, &GoBigger::AI::SimpleAIPlayer::aiPlayerDestroyed,
                    this, &GameView::onAIPlayerDestroyed);
        }
    }
}

void GameView::addRLAIPlayer()
{
    if (!m_gameManager) return;
    
    static int rlAiPlayerCount = 1;
    
    // 使用默认的RL模型路径 - 使用teamId=2区分于主玩家和普通AI
    // 注意：如果模型文件不存在，会自动回退到AGGRESSIVE策略
    QString modelPath = "assets/ai_models/default_rl_model.onnx";
    
    qDebug() << "Adding RL-AI player with model:" << modelPath;
    
    bool success = m_gameManager->addAIPlayerWithStrategy(2, 1000 + rlAiPlayerCount++, 
                                                         GoBigger::AI::AIStrategy::MODEL_BASED, modelPath);
    
    if (success && m_aiDebugWidget) {
        auto aiPlayers = m_gameManager->getAIPlayers();
        if (!aiPlayers.isEmpty()) {
            auto lastAI = aiPlayers.last();
            m_aiDebugWidget->addAIPlayer(lastAI);
            
            // 连接AI销毁信号
            connect(lastAI, &GoBigger::AI::SimpleAIPlayer::aiPlayerDestroyed,
                    this, &GameView::onAIPlayerDestroyed);
        }
    }
}

void GameView::startAllAI()
{
    if (m_gameManager) {
        m_gameManager->startAllAI();
    }
}

void GameView::stopAllAI()
{
    if (m_gameManager) {
        m_gameManager->stopAllAI();
    }
}

void GameView::removeAllAI()
{
    if (m_gameManager) {
        m_gameManager->removeAllAI();
        
        if (m_aiDebugWidget) {
            m_aiDebugWidget->clearAllAI();
        }
    }
}

void GameView::addAIPlayerWithDialog()
{
    if (!m_gameManager) return;

    QDialog dialog(this);
    dialog.setWindowTitle("添加AI玩家");
    dialog.setFixedSize(400, 300);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // AI策略选择
    QLabel* strategyLabel = new QLabel("AI策略类型:", &dialog);
    layout->addWidget(strategyLabel);

    QComboBox* strategyCombo = new QComboBox(&dialog);
    strategyCombo->addItem("随机移动", static_cast<int>(GoBigger::AI::AIStrategy::RANDOM));
    strategyCombo->addItem("食物猎手", static_cast<int>(GoBigger::AI::AIStrategy::FOOD_HUNTER));
    strategyCombo->addItem("攻击性策略", static_cast<int>(GoBigger::AI::AIStrategy::AGGRESSIVE));
    strategyCombo->addItem("模型驱动", static_cast<int>(GoBigger::AI::AIStrategy::MODEL_BASED));
    strategyCombo->setCurrentIndex(1); // 默认选择食物猎手
    layout->addWidget(strategyCombo);

    // 模型路径选择
    QLabel* modelLabel = new QLabel("RL模型路径 (仅模型驱动需要):", &dialog);
    layout->addWidget(modelLabel);

    QHBoxLayout* modelLayout = new QHBoxLayout();
    QLineEdit* modelPathEdit = new QLineEdit(&dialog);
    modelPathEdit->setPlaceholderText("选择或输入模型文件路径...");
    modelPathEdit->setEnabled(false); // 默认禁用

    QPushButton* browseButton = new QPushButton("浏览...", &dialog);
    browseButton->setEnabled(false); // 默认禁用

    modelLayout->addWidget(modelPathEdit);
    modelLayout->addWidget(browseButton);
    layout->addLayout(modelLayout);

    // 策略改变时的处理
    auto updateModelWidgets = [modelPathEdit, browseButton](int index) {
        bool isModelBased = (index == 3); // 模型驱动的索引
        modelPathEdit->setEnabled(isModelBased);
        browseButton->setEnabled(isModelBased);
        if (isModelBased && modelPathEdit->text().isEmpty()) {
            modelPathEdit->setText("assets/ai_models/default_rl_model.onnx");
        }
    };

    connect(strategyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), updateModelWidgets);

    // 浏览按钮处理
    connect(browseButton, &QPushButton::clicked, [&dialog, modelPathEdit]() {
        QString fileName = QFileDialog::getOpenFileName(
            &dialog,
            "选择RL模型文件",
            "assets/ai_models/",
            "模型文件 (*.onnx *.pt *.pth);;所有文件 (*.*)"
        );
        if (!fileName.isEmpty()) {
            modelPathEdit->setText(fileName);
        }
    });

    // 按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("确定", &dialog);
    QPushButton* cancelButton = new QPushButton("取消", &dialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        static int aiPlayerCount = 1;
        
        GoBigger::AI::AIStrategy strategy = static_cast<GoBigger::AI::AIStrategy>(
            strategyCombo->currentData().toInt()
        );
        
        QString modelPath = modelPathEdit->text();
        
        // 如果是模型驱动但没有模型路径，显示警告
        if (strategy == GoBigger::AI::AIStrategy::MODEL_BASED && modelPath.isEmpty()) {
            QMessageBox::warning(this, "警告", "模型驱动策略需要指定模型文件路径！");
            return;
        }
        
        // 根据策略类型分配不同的teamId
        int teamId = 10 + static_cast<int>(strategy); // 从10开始，避免与现有冲突
        
        bool success = m_gameManager->addAIPlayerWithStrategy(teamId, aiPlayerCount++, strategy, modelPath);
        
        if (success && m_aiDebugWidget) {
            auto aiPlayers = m_gameManager->getAIPlayers();
            if (!aiPlayers.isEmpty()) {
                auto lastAI = aiPlayers.last();
                m_aiDebugWidget->addAIPlayer(lastAI);
                
                // 连接AI销毁信号
                connect(lastAI, &GoBigger::AI::SimpleAIPlayer::aiPlayerDestroyed,
                        this, &GameView::onAIPlayerDestroyed);
            }
            qDebug() << "Successfully added AI player with strategy" << static_cast<int>(strategy);
        } else {
            QMessageBox::warning(this, "错误", "添加AI玩家失败！");
        }
    }
}

float GameView::getTotalPlayerScore() const
{
    if (!m_gameManager) return 0.0f;
    
    float totalScore = 0.0f;
    QVector<CloneBall*> allBalls = getAllPlayerBalls();
    
    for (CloneBall* ball : allBalls) {
        if (ball && !ball->isRemoved()) {
            totalScore += ball->radius() * ball->radius(); // 以面积计算分数
        }
    }
    
    return totalScore;
}

void GameView::drawBackground(QPainter *painter, const QRectF &rect)
{
    // 绘制网格背景
    painter->fillRect(rect, QColor(240, 248, 255)); // 淡蓝色背景
    
    painter->setPen(QPen(QColor(220, 220, 220), 1));
    
    // 绘制网格线
    const int gridSize = 50;
    int startX = static_cast<int>(rect.left() / gridSize) * gridSize;
    int startY = static_cast<int>(rect.top() / gridSize) * gridSize;
    
    for (int x = startX; x < rect.right(); x += gridSize) {
        painter->drawLine(x, rect.top(), x, rect.bottom());
    }
    
    for (int y = startY; y < rect.bottom(); y += gridSize) {
        painter->drawLine(rect.left(), y, rect.right(), y);
    }
}

QPointF GameView::calculatePlayerCentroidAll(const QVector<CloneBall*>& balls) const
{
    if (balls.isEmpty()) {
        return QPointF(0, 0);
    }
    
    QPointF centroid(0, 0);
    qreal totalMass = 0.0;
    
    for (CloneBall* ball : balls) {
        if (ball && !ball->isRemoved()) {
            qreal mass = ball->score();
            centroid += ball->pos() * mass;
            totalMass += mass;
        }
    }
    
    if (totalMass > 0.0) {
        return centroid / totalMass;
    }
    
    return QPointF(0, 0);
}