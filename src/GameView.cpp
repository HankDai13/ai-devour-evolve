#include "GameView.h"
#include "GameManager.h"
#include "CloneBall.h"
#include "SporeBall.h"
#include "BaseBall.h"
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QSet>
#include <QColor>
#include <QVector2D>
#include <QDebug>
#include <cmath>

GameView::GameView(QWidget *parent) 
    : QGraphicsView(parent)
    , m_gameManager(nullptr)
    , m_mainPlayer(nullptr)
    , m_inputTimer(nullptr)
    , m_zoomFactor(1.0)
    , m_followPlayer(true)
    , m_targetZoom(1.0)
    , m_minVisionRadius(100.0)     // 最小视野半径
    , m_maxVisionRadius(500.0)     // 最大视野半径
    , m_scaleUpRatio(1.8)          // 缩放比例，参考GoBigger
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
    // 创建场景
    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->setSceneRect(-500, -500, 1000, 1000); // 更大的游戏世界
    scene->setBackgroundBrush(QColor(240, 245, 250)); // 浅蓝灰色背景
    
    // 设置视图
    setScene(scene);
    setRenderHint(QPainter::Antialiasing);
    setCacheMode(QGraphicsView::CacheBackground);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setOptimizationFlags(QGraphicsView::DontSavePainterState | QGraphicsView::DontAdjustForAntialiasing);
    
    // 禁用滚动条，使用自定义相机控制
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragMode(QGraphicsView::NoDrag);
    
    // 启用键盘焦点
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    
    // 创建游戏管理器
    GameManager::Config config;
    config.gameBorder = Border(-500, 500, -500, 500);
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
        // 设置一个合理的初始质量，让玩家球更大一些
        m_mainPlayer->setMass(GoBiggerConfig::CELL_MIN_MASS); // 使用新的标准初始质量
        
        // 立即将视图中心设置到玩家位置
        centerOn(m_mainPlayer->pos());
        
        qDebug() << "Main player created with ID:" << m_mainPlayer->ballId() 
                 << "at position:" << m_mainPlayer->pos()
                 << "with radius:" << m_mainPlayer->radius()
                 << "with mass:" << m_mainPlayer->mass();
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
    
    // 处理特殊按键 - 立即处理，不依赖于其他按键状态
    switch (event->key()) {
        case Qt::Key_Space:
            qDebug() << "Space key detected - calling handleSplitAction";
            handleSplitAction();
            break;
        case Qt::Key_R:
            qDebug() << "R key detected - calling handleEjectAction";
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
    if (event->button() == Qt::LeftButton) {
        handleSplitAction();
    } else if (event->button() == Qt::RightButton) {
        handleEjectAction();
    }
    
    QGraphicsView::mousePressEvent(event);
}

void GameView::wheelEvent(QWheelEvent *event)
{
    // 缩放控制
    const qreal scaleFactor = 1.15;
    
    if (event->angleDelta().y() > 0) {
        // 放大
        m_zoomFactor *= scaleFactor;
        scale(scaleFactor, scaleFactor);
    } else {
        // 缩小
        m_zoomFactor /= scaleFactor;
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
    
    // 限制缩放范围
    m_zoomFactor = qBound(0.5, m_zoomFactor, 3.0);
}

void GameView::updateGameView()
{
    processInput();
    updateCamera();
}

void GameView::processInput()
{
    if (!m_mainPlayer || m_mainPlayer->isRemoved()) {
        return;
    }
    
    QVector2D moveDirection = calculateMoveDirection();
    
    if (moveDirection.length() > 0.01) {
        m_mainPlayer->setMoveDirection(moveDirection);
    } else {
        m_mainPlayer->setMoveDirection(QVector2D(0, 0));
    }
}

QVector2D GameView::calculateMoveDirection() const
{
    QVector2D direction(0, 0);
    
    // WASD 或方向键移动
    if (m_pressedKeys.contains(Qt::Key_W) || m_pressedKeys.contains(Qt::Key_Up)) {
        direction.setY(direction.y() - 1);
    }
    if (m_pressedKeys.contains(Qt::Key_S) || m_pressedKeys.contains(Qt::Key_Down)) {
        direction.setY(direction.y() + 1);
    }
    if (m_pressedKeys.contains(Qt::Key_A) || m_pressedKeys.contains(Qt::Key_Left)) {
        direction.setX(direction.x() - 1);
    }
    if (m_pressedKeys.contains(Qt::Key_D) || m_pressedKeys.contains(Qt::Key_Right)) {
        direction.setX(direction.x() + 1);
    }
    
    if (direction.length() > 0) {
        qDebug() << "Movement input detected:" << direction.x() << direction.y();
    }
    
    return direction.length() > 0 ? direction.normalized() : direction;
}

void GameView::updateCamera()
{
    if (!m_followPlayer || !m_mainPlayer || m_mainPlayer->isRemoved()) {
        return;
    }
    
    // 计算玩家质心位置
    QPointF centroid = calculatePlayerCentroid();
    centerOn(centroid);
    
    // 计算智能缩放
    calculateIntelligentZoom();
    
    // 应用平滑缩放
    adjustZoom();
}

void GameView::calculateIntelligentZoom()
{
    if (!m_mainPlayer || m_mainPlayer->isRemoved()) {
        return;
    }
    
    // 计算视野所需的最大半径，参考GoBigger算法
    qreal maxRadius = calculatePlayerRadius();
    
    // 应用最小视野限制
    maxRadius = qMax(maxRadius, m_minVisionRadius);
    
    // 计算缩放后的视野范围
    qreal scaledRadius = maxRadius * m_scaleUpRatio;
    
    // 基于视野范围计算目标缩放比例
    // 视窗大小的一半作为参考
    qreal viewportSize = qMin(width(), height()) * 0.4; // 40%的视窗大小
    m_targetZoom = viewportSize / scaledRadius;
    
    // 限制缩放范围
    m_targetZoom = qBound(0.2, m_targetZoom, 3.0);
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
    
    if (m_mainPlayer->canSplit()) {
        QVector2D splitDirection = calculateMoveDirection();
        if (splitDirection.length() < 0.01) {
            splitDirection = QVector2D(1, 0); // 默认向右分裂
        }
        
        QVector<CloneBall*> newBalls = m_mainPlayer->performSplit(splitDirection);
        qDebug() << "Split performed, created" << newBalls.size() << "new balls";
    } else {
        qDebug() << "Cannot split: insufficient score or cooldown";
    }
}

void GameView::handleEjectAction()
{
    qDebug() << "handleEjectAction called";
    
    if (!m_mainPlayer || m_mainPlayer->isRemoved()) {
        qDebug() << "No main player or player removed";
        return;
    }
    
    qDebug() << "Player canEject:" << m_mainPlayer->canEject() 
             << "Player mass:" << m_mainPlayer->mass();
    
    if (m_mainPlayer->canEject()) {
        // 获取当前移动方向，如果没有移动则使用默认方向
        QVector2D ejectDirection = calculateMoveDirection();
        
        qDebug() << "Current move direction:" << ejectDirection.x() << ejectDirection.y()
                 << "Pressed keys count:" << m_pressedKeys.size();
        
        // 如果没有移动方向，使用默认向右
        if (ejectDirection.length() < 0.01) {
            ejectDirection = QVector2D(1, 0); // 默认向右喷射
            qDebug() << "Using default direction (right)";
        } else {
            qDebug() << "Using movement direction for ejection";
        }
        
        qDebug() << "Ejecting spore in direction:" << ejectDirection.x() << ejectDirection.y();
        
        auto* spore = m_mainPlayer->ejectSpore(ejectDirection);
        if (spore) {
            qDebug() << "Spore ejected successfully at position:" 
                     << spore->pos().x() << spore->pos().y()
                     << "with velocity:" << spore->velocity().length();
        } else {
            qDebug() << "Failed to create spore";
        }
    } else {
        qDebug() << "Cannot eject: insufficient score";
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