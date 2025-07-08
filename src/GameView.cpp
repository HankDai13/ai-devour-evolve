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
#include <QPaintEvent>
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
#include <algorithm>
#include <limits>

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
    , m_lastTargetZoom(1.0)        // 🔥 初始化稳定性控制变量
    , m_lastCentroid(0, 0)
    , m_zoomDeadZone(0.05)         // 5%的缩放死区
    , m_centroidDeadZone(5.0)      // 5像素的质心移动死区
    , m_stableFrameCount(0)
    , m_requiredStableFrames(30)   // 需要30帧的稳定期
    , m_isInitialStabilizing(true) // 开局进入稳定模式
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
    qDebug() << "🎮 initializePlayer called";
    
    // 检查GameManager是否已创建
    if (!m_gameManager) {
        qDebug() << "🎮 GameManager not created yet, cannot initialize player!";
        return;
    }
    
    // 🔥 修复：更严格的重复创建检查
    if (m_mainPlayer && !m_mainPlayer->isRemoved()) {
        qDebug() << "🎮 Main player already exists, skipping initialization. BallId:" << m_mainPlayer->ballId();
        return;
    }
    
    // 🔥 修复：检查GameManager中所有现有玩家，防止重复创建
    QVector<CloneBall*> allPlayers = m_gameManager->getPlayers();
    for (CloneBall* player : allPlayers) {
        if (player && !player->isRemoved() && 
            player->teamId() == GoBiggerConfig::HUMAN_TEAM_ID && player->playerId() == 0) {
            qDebug() << "Human player already exists in GameManager, reusing it. Player ID:" << player->ballId();
            m_mainPlayer = player;
            
            // 重新设置视角
            QPointF playerPos = m_mainPlayer->pos();
            float initialRadius = m_mainPlayer->radius();
            float initialVisionSize = initialRadius * 12.0f;
            float viewportSize = std::min(width(), height()) * 0.8f;
            qreal initialZoom = viewportSize / initialVisionSize;
            initialZoom = qBound(0.5, initialZoom, 1.5);
            
            resetTransform();
            scale(initialZoom, initialZoom);
            m_zoomFactor = initialZoom;
            m_targetZoom = initialZoom;
            m_lastTargetZoom = initialZoom;
            centerOn(playerPos);
            
            m_isInitialStabilizing = true;
            m_stableFrameCount = 0;
            
            qDebug() << "Reused existing human player with ball ID:" << m_mainPlayer->ballId();
            return;
        }
    }
    
    // 🔥 修复：确保游戏已启动，但只启动一次
    if (!m_gameManager->isGameRunning()) {
        m_gameManager->startGame();
        qDebug() << "Game started";
    }
    
    // 🔥 再次检查，确保没有其他代码路径创建了玩家
    allPlayers = m_gameManager->getPlayers();
    for (CloneBall* player : allPlayers) {
        if (player && !player->isRemoved() && 
            player->teamId() == GoBiggerConfig::HUMAN_TEAM_ID && player->playerId() == 0) {
            qDebug() << "Found human player created by startGame, reusing it. Player ID:" << player->ballId();
            m_mainPlayer = player;
            return;
        }
    }
    
    // 创建主玩家 - 只有在确实没有的情况下才创建
    qDebug() << "Creating new human player...";
    m_mainPlayer = m_gameManager->createPlayer(GoBiggerConfig::HUMAN_TEAM_ID, 0, QPointF(0, 0));
    
    if (m_mainPlayer) {
        // 设置一个合理的初始分数，让玩家球更大一些
        m_mainPlayer->setScore(GoBiggerConfig::CELL_INIT_SCORE); // 使用新的标准初始分数
        
        // 🔥 初始视角稳定设置
        QPointF playerPos = m_mainPlayer->pos();
        m_lastCentroid = playerPos;
        
        // 立即设置合理的初始缩放，避免后续计算导致的跳跃
        float initialRadius = m_mainPlayer->radius();
        float initialVisionSize = initialRadius * 12.0f; // 固定12倍半径的初始视野
        float viewportSize = std::min(width(), height()) * 0.8f;
        qreal initialZoom = viewportSize / initialVisionSize;
        initialZoom = qBound(0.5, initialZoom, 1.5);
        
        // 直接设置初始缩放，避免过渡动画
        resetTransform();
        scale(initialZoom, initialZoom);
        m_zoomFactor = initialZoom;
        m_targetZoom = initialZoom;
        m_lastTargetZoom = initialZoom;
        
        // 将视图中心设置到玩家位置
        centerOn(playerPos);
        
        // 🔥 确保进入初始稳定模式
        m_isInitialStabilizing = true;
        m_stableFrameCount = 0;
        
        qDebug() << "Main player created with ID:" << m_mainPlayer->ballId() 
                 << "at position:" << m_mainPlayer->pos()
                 << "with radius:" << m_mainPlayer->radius()
                 << "with score:" << m_mainPlayer->score()
                 << "initial zoom:" << initialZoom;
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
        connect(m_gameManager, &GameManager::gameOver, this, &GameView::onGameOver);
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
        // 清空主玩家引用（将在resetGame后重新创建）
        m_mainPlayer = nullptr;
        
        m_gameManager->resetGame();
        
        // 🔥 重置视角稳定性状态
        m_isInitialStabilizing = true;
        m_stableFrameCount = 0;
        m_lastCentroid = QPointF(0, 0);
        m_lastTargetZoom = 1.0;
        
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
    
    // 🔥 触发UI层重绘，确保排行榜及时更新
    viewport()->update();
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
    QPointF currentCentroid = calculatePlayerCentroidAll(allPlayerBalls);
    
    // 🔥 质心稳定性检查 - 避免微小移动导致的抖动
    bool centroidStable = true;
    if (!m_isInitialStabilizing) {
        float centroidDistance = QLineF(currentCentroid, m_lastCentroid).length();
        centroidStable = (centroidDistance < m_centroidDeadZone);
    }
    
    // 只有质心移动足够大时才更新
    if (centroidStable && !m_isInitialStabilizing) {
        // 保持当前位置，避免小幅度跳动
    } else {
        // 🔥 平滑质心跟随 - 使用插值而不是直接跳转
        if (!m_isInitialStabilizing && m_lastCentroid != QPointF(0, 0)) {
            // 计算插值后的位置
            float lerpFactor = 0.15f; // 15%的插值速度
            QPointF smoothCentroid = m_lastCentroid + (currentCentroid - m_lastCentroid) * lerpFactor;
            centerOn(smoothCentroid);
            m_lastCentroid = smoothCentroid;
        } else {
            // 初始化或首次设置，直接使用目标位置
            centerOn(currentCentroid);
            m_lastCentroid = currentCentroid;
        }
    }
    
    // 计算智能缩放
    qreal oldTargetZoom = m_targetZoom;
    calculateIntelligentZoomGoBigger(allPlayerBalls);
    
    // 🔥 缩放稳定性检查 - 避免频繁的微小缩放变化
    bool zoomStable = true;
    if (!m_isInitialStabilizing) {
        float zoomChangeRatio = std::abs(m_targetZoom - m_lastTargetZoom) / m_lastTargetZoom;
        zoomStable = (zoomChangeRatio < m_zoomDeadZone);
    }
    
    // 检查是否进入稳定状态
    if (centroidStable && zoomStable && !m_isInitialStabilizing) {
        m_stableFrameCount++;
    } else {
        m_stableFrameCount = 0;
        m_isInitialStabilizing = false; // 一旦有变化就退出初始稳定模式
    }
    
    // 只有在不稳定或者稳定时间足够长时才调整缩放
    if (!zoomStable || m_stableFrameCount >= m_requiredStableFrames || m_isInitialStabilizing) {
        m_lastTargetZoom = m_targetZoom;
        adjustZoom();
        
        if (m_stableFrameCount >= m_requiredStableFrames) {
            m_stableFrameCount = 0; // 重置计数
        }
    }
    
    // 初始稳定期结束条件
    if (m_isInitialStabilizing && allPlayerBalls.size() == 1) {
        static int initialFrames = 0;
        initialFrames++;
        if (initialFrames > 60) { // 1秒后结束初始稳定期
            m_isInitialStabilizing = false;
            initialFrames = 0;
        }
    }
}

void GameView::calculateIntelligentZoomGoBigger(const QVector<CloneBall*>& allPlayerBalls)
{
    if (allPlayerBalls.isEmpty()) {
        return;
    }
    
    // 🔥 单球特殊处理 - 开局稳定性优化
    if (allPlayerBalls.size() == 1 && m_isInitialStabilizing) {
        CloneBall* ball = allPlayerBalls.first();
        if (ball && !ball->isRemoved()) {
            // 开局时使用固定的合理缩放，避免抖动
            float ballRadius = ball->radius();
            float fixedVisionSize = std::max(ballRadius * 12.0f, 400.0f); // 最小400像素视野
            float viewportSize = std::min(width(), height()) * 0.8f;
            m_targetZoom = viewportSize / fixedVisionSize;
            m_targetZoom = qBound(0.5, m_targetZoom, 1.5); // 限制初始缩放范围
            return;
        }
    }
    
    // GoBigger风格视野计算：
    // 1. 计算所有球的最小外接矩形
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    
    float maxRadius = 0;
    float totalScore = 0; // 用于权重计算
    
    for (CloneBall* ball : allPlayerBalls) {
        if (!ball || ball->isRemoved()) continue;
        
        QPointF pos = ball->pos();
        float radius = ball->radius();
        float score = ball->score();
        
        minX = std::min(minX, static_cast<float>(pos.x() - radius));
        maxX = std::max(maxX, static_cast<float>(pos.x() + radius));
        minY = std::min(minY, static_cast<float>(pos.y() - radius));
        maxY = std::max(maxY, static_cast<float>(pos.y() + radius));
        
        maxRadius = std::max(maxRadius, radius);
        totalScore += score;
    }
    
    // 2. 计算外接矩形的大小
    float rectWidth = maxX - minX;
    float rectHeight = maxY - minY;
    float maxDimension = std::max(rectWidth, rectHeight);
    
    // 3. 🔥 智能最小视野计算 - 根据球的状态动态调整
    float baseVisionMultiplier = 10.0f; // 基础视野倍数
    
    // 根据球的数量调整视野：球越多视野越大
    if (allPlayerBalls.size() > 1) {
        baseVisionMultiplier += allPlayerBalls.size() * 2.0f;
    }
    
    // 根据总分数调整视野：分数越高视野越大（但有上限）
    float scoreMultiplier = 1.0f + std::min(totalScore / 1000.0f, 2.0f);
    
    float minVisionSize = maxRadius * baseVisionMultiplier * scoreMultiplier;
    float requiredVisionSize = std::max(maxDimension, minVisionSize);
    
    // 4. 🔥 渐进式放大系数 - 避免突然的视野变化
    float dynamicScaleRatio = m_scaleUpRatio;
    if (!m_isInitialStabilizing) {
        // 根据当前缩放级别动态调整放大系数
        float currentZoom = transform().m11();
        if (currentZoom > 1.0f) {
            dynamicScaleRatio = qMax(1.5f, m_scaleUpRatio * (2.0f - currentZoom));
        }
    }
    
    requiredVisionSize *= dynamicScaleRatio;
    
    // 5. 计算目标缩放比例
    float viewportSize = std::min(width(), height()) * 0.8f; // 80%的视口利用率
    qreal newTargetZoom = viewportSize / requiredVisionSize;
    
    // 6. 🔥 缩放变化限制 - 防止剧烈缩放变化
    if (!m_isInitialStabilizing && m_lastTargetZoom > 0) {
        float maxZoomChangePerFrame = 0.05f; // 每帧最大5%的缩放变化
        float zoomChangeRatio = newTargetZoom / m_lastTargetZoom;
        
        if (zoomChangeRatio > 1.0f + maxZoomChangePerFrame) {
            newTargetZoom = m_lastTargetZoom * (1.0f + maxZoomChangePerFrame);
        } else if (zoomChangeRatio < 1.0f - maxZoomChangePerFrame) {
            newTargetZoom = m_lastTargetZoom * (1.0f - maxZoomChangePerFrame);
        }
    }
    
    // 7. 更严格的缩放范围限制
    m_targetZoom = qBound(0.3, newTargetZoom, 2.0);
    
    qDebug() << "Vision calculation - balls:" << allPlayerBalls.size()
             << "rectSize:" << maxDimension 
             << "maxRadius:" << maxRadius 
             << "requiredVision:" << requiredVisionSize 
             << "targetZoom:" << m_targetZoom
             << "isStabilizing:" << m_isInitialStabilizing;
}

int GameView::assignTeamForNewAI()
{
    if (!m_gameManager) return 1;
    
    // 🔥 队伍分配规则：每队最多2人，依次填满
    // 队伍0：人类玩家 + 第1个AI
    // 队伍1：第2个AI + 第3个AI  
    // 队伍2：第4个AI + 第5个AI
    // 以此类推...
    
    auto aiPlayers = m_gameManager->getAIPlayers();
    int totalAI = aiPlayers.size();
    
    // 统计各队当前人数
    QMap<int, int> teamPlayerCount;
    
    // 人类玩家占用队伍0的1个位置
    teamPlayerCount[0] = 1;
    
    // 统计现有AI的队伍分布
    for (auto* ai : aiPlayers) {
        if (ai && ai->getPlayerBall()) {
            int teamId = ai->getPlayerBall()->teamId();
            teamPlayerCount[teamId]++;
        }
    }
    
    // 从队伍0开始，找到第一个未满的队伍
    for (int teamId = 0; teamId < GoBiggerConfig::MAX_TEAMS; ++teamId) {
        int currentCount = teamPlayerCount.value(teamId, 0);
        if (currentCount < GoBiggerConfig::MAX_PLAYERS_PER_TEAM) {
            qDebug() << "Assigning new AI to team" << teamId 
                     << "(current count:" << currentCount << ")";
            return teamId;
        }
    }
    
    // 如果所有队伍都满了，循环分配
    int teamId = totalAI % GoBiggerConfig::MAX_TEAMS;
    qDebug() << "All teams full, cycling to team" << teamId;
    return teamId;
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
    // 获取当前缩放
    qreal currentZoom = transform().m11();
    qreal zoomDiff = m_targetZoom - currentZoom;
    
    // 🔥 更精确的死区检查
    qreal minZoomDiff = 0.001; // 最小有意义的缩放差异
    if (std::abs(zoomDiff) <= minZoomDiff) {
        return; // 差异太小，不需要调整
    }
    
    // 🔥 智能过渡速度 - 根据差异大小调整
    qreal transitionSpeed;
    qreal zoomRatio = std::abs(zoomDiff) / currentZoom;
    
    if (m_isInitialStabilizing) {
        // 初始稳定期：更慢的过渡
        transitionSpeed = 0.03;
    } else if (zoomRatio > 0.2) {
        // 大变化：较快的过渡
        transitionSpeed = 0.12;
    } else if (zoomRatio > 0.1) {
        // 中等变化：中等过渡
        transitionSpeed = 0.08;
    } else {
        // 小变化：慢过渡，避免抖动
        transitionSpeed = 0.04;
    }
    
    // 🔥 使用更平滑的插值函数（easeInOutQuad）
    qreal t = transitionSpeed;
    qreal smoothT = t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;
    
    qreal newZoom = currentZoom + zoomDiff * smoothT;
    
    // 确保缩放在合理范围内
    newZoom = qBound(0.3, newZoom, 2.0);
    
    // 🔥 只有当变化足够大时才应用变换
    if (std::abs(newZoom - currentZoom) > 0.0005) {
        // 重置变换并应用新缩放
        resetTransform();
        scale(newZoom, newZoom);
        m_zoomFactor = newZoom;
        
        qDebug() << "Zoom adjusted - current:" << currentZoom 
                 << "target:" << m_targetZoom 
                 << "new:" << newZoom 
                 << "speed:" << transitionSpeed;
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

QColor GameView::getTeamColor(int teamId) const
{
    if (teamId >= 0 && teamId < GoBiggerConfig::TEAM_COLORS.size()) {
        return GoBiggerConfig::TEAM_COLORS[teamId];
    }
    return Qt::gray; // 默认颜色
}

void GameView::paintEvent(QPaintEvent *event)
{
    // 首先调用基类的paintEvent
    QGraphicsView::paintEvent(event);

    // 然后在UI层绘制排行榜
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);
    drawTeamLeaderboard(&painter);
}

void GameView::drawTeamLeaderboard(QPainter* painter)
{
    if (!painter) return;

    // 1. 获取并排序队伍分数
    QMap<int, float> teamScores = calculateTeamScores();
    if (teamScores.isEmpty()) return;

    QVector<QPair<int, float>> sortedScores;
    for (auto it = teamScores.constBegin(); it != teamScores.constEnd(); ++it) {
        sortedScores.append(qMakePair(it.key(), it.value()));
    }
    std::sort(sortedScores.begin(), sortedScores.end(), [](const QPair<int, float>& a, const QPair<int, float>& b) {
        return a.second > b.second;
    });

    // 2. 设置排行榜样式
    int width = 220;
    int height = 40 + sortedScores.size() * 30;
    int margin = 15;
    QRectF leaderboardRect(viewport()->width() - width - margin, margin, width, height);

    // 绘制半透明背景
    painter->setBrush(QColor(0, 0, 0, 100));
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(leaderboardRect, 10, 10);

    // 3. 绘制标题
    painter->setPen(Qt::white);
    QFont titleFont("Arial", 14, QFont::Bold);
    painter->setFont(titleFont);
    painter->drawText(leaderboardRect.adjusted(0, 10, 0, 0), Qt::AlignHCenter | Qt::AlignTop, "Team Leaderboard");

    // 4. 绘制每个队伍的分数
    QFont entryFont("Arial", 12);
    painter->setFont(entryFont);
    int yPos = leaderboardRect.top() + 45;

    for (const auto& pair : sortedScores) {
        int teamId = pair.first;
        float score = pair.second;

        // 获取队伍颜色
        QColor teamColor = getTeamColor(teamId);
        painter->setPen(teamColor);

        // 构造队伍名称
        QString teamName = (teamId == GoBiggerConfig::HUMAN_TEAM_ID) ? "Your Team" : QString("AI Team %1").arg(teamId);

        // 绘制队伍名称和分数
        QRectF textRect(leaderboardRect.left() + 15, yPos, leaderboardRect.width() - 30, 25);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, teamName);
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(static_cast<int>(score)));

        yPos += 30;
    }
}

QMap<int, float> GameView::calculateTeamScores() const
{
    if (!m_gameManager) return {};
    return m_gameManager->getAllTeamScores();
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
    
    // 🔥 重置视角稳定性状态
    m_isInitialStabilizing = true;
    m_stableFrameCount = 0;
    m_lastCentroid = QPointF(0, 0);
    m_lastTargetZoom = 1.0;
    
    // 重置相机
    resetTransform();
    m_zoomFactor = 1.0;
    m_targetZoom = 1.0;
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
    
    // 🔥 使用新的队伍分配逻辑
    int teamId = assignTeamForNewAI();
    
    // 使用分配的队伍ID添加AI玩家，默认使用FOOD_HUNTER策略
    bool success = m_gameManager->addAIPlayerWithStrategy(teamId, aiPlayerCount++, 
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
    
    // 🔥 使用统一的队伍分配逻辑
    int teamId = assignTeamForNewAI();
    
    // 使用默认的RL模型路径
    QString modelPath = "assets/ai_models/default_rl_model.onnx";
    
    qDebug() << "Adding RL-AI player to team" << teamId << "with model:" << modelPath;
    
    bool success = m_gameManager->addAIPlayerWithStrategy(teamId, 1000 + rlAiPlayerCount++, 
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
        
        // 🔥 使用统一的队伍分配逻辑
        int teamId = assignTeamForNewAI();
        
        qDebug() << "Adding AI with strategy" << static_cast<int>(strategy) 
                 << "to team" << teamId;
        
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
            totalScore += ball->score(); // 🔥 修正：使用实际分数而不是面积
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

void GameView::onGameOver(int winningTeamId)
{
    showGameOverScreen(winningTeamId);
}

void GameView::showGameOverScreen(int winningTeamId)
{
    // Create a semi-transparent overlay
    QGraphicsRectItem* overlay = new QGraphicsRectItem(sceneRect());
    overlay->setBrush(QColor(0, 0, 0, 180));
    scene()->addItem(overlay);

    // Create the game over text
    QGraphicsTextItem* textItem = new QGraphicsTextItem();
    if (winningTeamId == m_mainPlayer->teamId()) {
        textItem->setHtml("<h1 style='color: #00FF00;'>VICTORY</h1>");
    } else {
        textItem->setHtml("<h1 style='color: #FF0000;'>GAME OVER</h1>");
    }

    QFont font("Arial", 48, QFont::Bold);
    textItem->setFont(font);
    textItem->setPos(sceneRect().center().x() - textItem->boundingRect().width() / 2, 
                     sceneRect().center().y() - textItem->boundingRect().height() / 2);
    scene()->addItem(textItem);
}