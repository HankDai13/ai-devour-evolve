#include "GameManager.h"
#include "CloneBall.h"
#include "FoodBall.h"
#include "SporeBall.h"
#include "ThornsBall.h"
#include <QGraphicsScene>
#include <QDebug>
#include <cmath>

GameManager::GameManager(QGraphicsScene* scene, const Config& config, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_config(config)
    , m_gameRunning(false)
    , m_gameTimer(nullptr)
    , m_foodTimer(nullptr)
    , m_thornsTimer(nullptr)
    , m_nextBallId(1)
{
    initializeTimers();
}

GameManager::~GameManager()
{
    clearAllBalls();
    
    if (m_gameTimer) {
        m_gameTimer->stop();
        delete m_gameTimer;
    }
    if (m_foodTimer) {
        m_foodTimer->stop();
        delete m_foodTimer;
    }
    if (m_thornsTimer) {
        m_thornsTimer->stop();
        delete m_thornsTimer;
    }
}

void GameManager::startGame()
{
    if (!m_gameRunning) {
        m_gameRunning = true;
        m_gameTimer->start();
        m_foodTimer->start();
        m_thornsTimer->start();
        
        // 初始生成一些食物和荆棘
        for (int i = 0; i < m_config.maxFoodCount / 2; ++i) {
            spawnFood();
        }
        for (int i = 0; i < m_config.maxThornsCount / 2; ++i) {
            spawnThorns();
        }
        
        emit gameStarted();
        qDebug() << "Game started";
    }
}

void GameManager::pauseGame()
{
    if (m_gameRunning) {
        m_gameRunning = false;
        m_gameTimer->stop();
        m_foodTimer->stop();
        m_thornsTimer->stop();
        
        emit gamePaused();
        qDebug() << "Game paused";
    }
}

void GameManager::resetGame()
{
    pauseGame();
    clearAllBalls();
    m_nextBallId = 1;
    
    emit gameReset();
    qDebug() << "Game reset";
}

CloneBall* GameManager::createPlayer(int teamId, int playerId, const QPointF& position)
{
    // 检查是否已存在相同的玩家
    for (CloneBall* player : m_players) {
        if (player->teamId() == teamId && player->playerId() == playerId) {
            qDebug() << "Player already exists:" << teamId << playerId;
            return player;
        }
    }
    
    QPointF spawnPos = position.isNull() ? generateRandomPosition() : position;
    
    CloneBall* player = new CloneBall(
        getNextBallId(),
        spawnPos,
        m_config.gameBorder,
        teamId,
        playerId
    );
    
    addBall(player);
    m_players.append(player);
    
    // 连接玩家特有的信号
    connect(player, &CloneBall::splitPerformed, this, &GameManager::handlePlayerSplit);
    connect(player, &CloneBall::sporeEjected, this, &GameManager::handleSporeEjected);
    
    emit playerAdded(player);
    qDebug() << "Player created:" << teamId << playerId << "at" << spawnPos;
    
    return player;
}

void GameManager::removePlayer(CloneBall* player)
{
    if (player && m_players.contains(player)) {
        m_players.removeOne(player);
        removeBall(player);
        
        emit playerRemoved(player);
        qDebug() << "Player removed:" << player->teamId() << player->playerId();
    }
}

CloneBall* GameManager::getPlayer(int teamId, int playerId) const
{
    for (CloneBall* player : m_players) {
        if (player->teamId() == teamId && player->playerId() == playerId) {
            return player;
        }
    }
    return nullptr;
}

void GameManager::addBall(BaseBall* ball)
{
    if (!ball) return;
    
    m_allBalls.insert(ball->ballId(), ball);
    
    // 根据类型添加到相应的列表
    switch (ball->ballType()) {
        case BaseBall::CLONE_BALL:
            // 玩家球已在createPlayer中处理
            break;
        case BaseBall::FOOD_BALL:
            m_foodBalls.append(static_cast<FoodBall*>(ball));
            break;
        case BaseBall::SPORE_BALL:
            m_sporeBalls.append(static_cast<SporeBall*>(ball));
            break;
        case BaseBall::THORNS_BALL:
            m_thornsBalls.append(static_cast<ThornsBall*>(ball));
            break;
    }
    
    // 添加到场景
    if (m_scene && !m_scene->items().contains(ball)) {
        m_scene->addItem(ball);
    }
    
    // 连接信号
    connectBallSignals(ball);
    
    emit ballAdded(ball);
}

void GameManager::removeBall(BaseBall* ball)
{
    if (!ball) return;
    
    m_allBalls.remove(ball->ballId());
    
    // 从相应的列表中移除
    switch (ball->ballType()) {
        case BaseBall::CLONE_BALL:
            // 玩家球在removePlayer中处理
            break;
        case BaseBall::FOOD_BALL:
            m_foodBalls.removeOne(static_cast<FoodBall*>(ball));
            break;
        case BaseBall::SPORE_BALL:
            m_sporeBalls.removeOne(static_cast<SporeBall*>(ball));
            break;
        case BaseBall::THORNS_BALL:
            m_thornsBalls.removeOne(static_cast<ThornsBall*>(ball));
            break;
    }
    
    // 断开信号
    disconnectBallSignals(ball);
    
    // 从场景中移除
    removeFromScene(ball);
    
    emit ballRemoved(ball);
}

QVector<BaseBall*> GameManager::getAllBalls() const
{
    return m_allBalls.values().toVector();
}

QVector<BaseBall*> GameManager::getBallsNear(const QPointF& position, qreal radius) const
{
    QVector<BaseBall*> nearbyBalls;
    
    for (BaseBall* ball : m_allBalls) {
        if (ball && !ball->isRemoved()) {
            QPointF ballPos = ball->pos();
            qreal distance = std::sqrt(std::pow(position.x() - ballPos.x(), 2) + 
                                      std::pow(position.y() - ballPos.y(), 2));
            if (distance <= radius) {
                nearbyBalls.append(ball);
            }
        }
    }
    
    return nearbyBalls;
}

void GameManager::initializeTimers()
{
    // 游戏更新定时器
    m_gameTimer = new QTimer(this);
    connect(m_gameTimer, &QTimer::timeout, this, &GameManager::updateGame);
    m_gameTimer->setInterval(m_config.gameUpdateInterval);
    
    // 食物生成定时器
    m_foodTimer = new QTimer(this);
    connect(m_foodTimer, &QTimer::timeout, this, &GameManager::spawnFood);
    m_foodTimer->setInterval(2000); // 每2秒生成1个食物，降低生成速度
    
    // 荆棘生成定时器
    m_thornsTimer = new QTimer(this);
    connect(m_thornsTimer, &QTimer::timeout, this, &GameManager::spawnThorns);
    m_thornsTimer->setInterval(30000 / m_config.thornsSpawnRate); // 每30秒生成指定数量
}

void GameManager::connectBallSignals(BaseBall* ball)
{
    if (!ball) return;
    
    connect(ball, &BaseBall::ballRemoved, this, &GameManager::handleBallRemoved);
    
    // 连接特定类型的信号
    if (ball->ballType() == BaseBall::THORNS_BALL) {
        ThornsBall* thorns = static_cast<ThornsBall*>(ball);
        connect(thorns, &ThornsBall::thornsCollision, this, &GameManager::handleThornsCollision);
    }
}

void GameManager::disconnectBallSignals(BaseBall* ball)
{
    if (!ball) return;
    
    disconnect(ball, nullptr, this, nullptr);
}

QPointF GameManager::generateRandomPosition() const
{
    QRandomGenerator* rng = QRandomGenerator::global();
    
    qreal x = m_config.gameBorder.minx + 
              (m_config.gameBorder.maxx - m_config.gameBorder.minx) * rng->generateDouble();
    qreal y = m_config.gameBorder.miny + 
              (m_config.gameBorder.maxy - m_config.gameBorder.miny) * rng->generateDouble();
    
    return QPointF(x, y);
}

QPointF GameManager::generateRandomFoodPosition() const
{
    return generateRandomPosition();
}

QPointF GameManager::generateRandomThornsPosition() const
{
    // 荆棘生成位置避开玩家球附近
    QPointF pos;
    int attempts = 0;
    const int maxAttempts = 50;
    
    do {
        pos = generateRandomPosition();
        bool tooClose = false;
        
        for (CloneBall* player : m_players) {
            if (player && !player->isRemoved()) {
                qreal distance = std::sqrt(std::pow(pos.x() - player->pos().x(), 2) + 
                                          std::pow(pos.y() - player->pos().y(), 2));
                if (distance < 100.0) { // 至少距离玩家100像素
                    tooClose = true;
                    break;
                }
            }
        }
        
        if (!tooClose) break;
        attempts++;
    } while (attempts < maxAttempts);
    
    return pos;
}

void GameManager::updateGame()
{
    if (!m_gameRunning) return;
    
    // 检查碰撞
    checkCollisions();
    
    // 清理已移除的球
    QVector<BaseBall*> ballsToRemove;
    for (BaseBall* ball : m_allBalls) {
        if (ball && ball->isRemoved()) {
            ballsToRemove.append(ball);
        }
    }
    
    for (BaseBall* ball : ballsToRemove) {
        removeBall(ball);
        ball->deleteLater();
    }
}

void GameManager::spawnFood()
{
    if (m_foodBalls.size() < m_config.maxFoodCount) {
        // 尝试多次找到合适的位置
        bool positionFound = false;
        QPointF pos;
        int attempts = 0;
        const int maxAttempts = 20;
        
        while (!positionFound && attempts < maxAttempts) {
            pos = generateRandomFoodPosition();
            
            // 检查该位置附近的食物密度
            int nearbyFoodCount = 0;
            for (FoodBall* food : m_foodBalls) {
                if (food && !food->isRemoved()) {
                    qreal distance = std::sqrt(
                        std::pow(pos.x() - food->pos().x(), 2) + 
                        std::pow(pos.y() - food->pos().y(), 2)
                    );
                    if (distance < m_config.foodDensityRadius) {
                        nearbyFoodCount++;
                    }
                }
            }
            
            // 如果密度未超过限制，则使用该位置
            if (nearbyFoodCount < m_config.maxFoodInDensityArea) {
                positionFound = true;
            }
            
            attempts++;
        }
        
        if (positionFound) {
            FoodBall* food = new FoodBall(getNextBallId(), pos, m_config.gameBorder);
            addBall(food);
        }
    }
}

void GameManager::spawnThorns()
{
    if (m_thornsBalls.size() < m_config.maxThornsCount) {
        QPointF pos = generateRandomThornsPosition();
        ThornsBall* thorns = new ThornsBall(getNextBallId(), pos, m_config.gameBorder);
        addBall(thorns);
    }
}

void GameManager::checkCollisions()
{
    QVector<BaseBall*> allBalls = getAllBalls();
    
    for (int i = 0; i < allBalls.size(); ++i) {
        for (int j = i + 1; j < allBalls.size(); ++j) {
            BaseBall* ball1 = allBalls[i];
            BaseBall* ball2 = allBalls[j];
            
            if (ball1 && ball2 && !ball1->isRemoved() && !ball2->isRemoved()) {
                if (ball1->collidesWith(ball2)) {
                    checkCollisionsBetween(ball1, ball2);
                }
            }
        }
    }
}

void GameManager::checkCollisionsBetween(BaseBall* ball1, BaseBall* ball2)
{
    // 处理不同类型球之间的碰撞
    
    // 玩家球 vs 食物球
    if ((ball1->ballType() == BaseBall::CLONE_BALL && ball2->ballType() == BaseBall::FOOD_BALL) ||
        (ball1->ballType() == BaseBall::FOOD_BALL && ball2->ballType() == BaseBall::CLONE_BALL)) {
        
        CloneBall* player = (ball1->ballType() == BaseBall::CLONE_BALL) ? 
                           static_cast<CloneBall*>(ball1) : static_cast<CloneBall*>(ball2);
        FoodBall* food = (ball1->ballType() == BaseBall::FOOD_BALL) ? 
                        static_cast<FoodBall*>(ball1) : static_cast<FoodBall*>(ball2);
        
        qDebug() << "Player-Food collision detected!" 
                 << "Player radius:" << player->radius() 
                 << "Food radius:" << food->radius()
                 << "Player mass:" << player->mass()
                 << "Food mass:" << food->mass();
        
        if (player->canEat(food)) {
            qDebug() << "Player can eat food, eating now...";
            player->eat(food);
        } else {
            qDebug() << "Player cannot eat food - size ratio too small";
        }
    }
    
    // 玩家球 vs 孢子球
    else if ((ball1->ballType() == BaseBall::CLONE_BALL && ball2->ballType() == BaseBall::SPORE_BALL) ||
             (ball1->ballType() == BaseBall::SPORE_BALL && ball2->ballType() == BaseBall::CLONE_BALL)) {
        
        CloneBall* player = (ball1->ballType() == BaseBall::CLONE_BALL) ? 
                           static_cast<CloneBall*>(ball1) : static_cast<CloneBall*>(ball2);
        SporeBall* spore = (ball1->ballType() == BaseBall::SPORE_BALL) ? 
                          static_cast<SporeBall*>(ball1) : static_cast<SporeBall*>(ball2);
        
        // 检查是否是敌方孢子
        if (player->teamId() != spore->teamId() && player->canEat(spore)) {
            player->eat(spore);
        }
    }
    
    // 玩家球 vs 玩家球
    else if (ball1->ballType() == BaseBall::CLONE_BALL && ball2->ballType() == BaseBall::CLONE_BALL) {
        CloneBall* player1 = static_cast<CloneBall*>(ball1);
        CloneBall* player2 = static_cast<CloneBall*>(ball2);
        
        // 不同队伍的玩家球可以互相吞噬
        if (player1->teamId() != player2->teamId()) {
            if (player1->canEat(player2)) {
                player1->eat(player2);
            } else if (player2->canEat(player1)) {
                player2->eat(player1);
            }
        }
    }
    
    // 玩家球 vs 荆棘球
    else if ((ball1->ballType() == BaseBall::CLONE_BALL && ball2->ballType() == BaseBall::THORNS_BALL) ||
             (ball1->ballType() == BaseBall::THORNS_BALL && ball2->ballType() == BaseBall::CLONE_BALL)) {
        
        CloneBall* player = (ball1->ballType() == BaseBall::CLONE_BALL) ? 
                           static_cast<CloneBall*>(ball1) : static_cast<CloneBall*>(ball2);
        ThornsBall* thorns = (ball1->ballType() == BaseBall::THORNS_BALL) ? 
                            static_cast<ThornsBall*>(ball1) : static_cast<ThornsBall*>(ball2);
        
        thorns->causeCollisionDamage(player);
    }
}

void GameManager::handleBallRemoved(BaseBall* ball)
{
    if (ball) {
        removeBall(ball);
    }
}

void GameManager::handlePlayerSplit(CloneBall* originalBall, const QVector<CloneBall*>& newBalls)
{
    // 将新分裂的球添加到管理器
    for (CloneBall* newBall : newBalls) {
        addBall(newBall);
        m_players.append(newBall);
        
        // 连接新球的信号
        connect(newBall, &CloneBall::splitPerformed, this, &GameManager::handlePlayerSplit);
        connect(newBall, &CloneBall::sporeEjected, this, &GameManager::handleSporeEjected);
    }
    
    qDebug() << "Player split: original" << originalBall->ballId() << "created" << newBalls.size() << "new balls";
}

void GameManager::handleSporeEjected(CloneBall* ball, SporeBall* spore)
{
    addBall(spore);
    qDebug() << "Spore ejected by player" << ball->ballId();
}

void GameManager::handleThornsCollision(ThornsBall* thorns, CloneBall* ball)
{
    Q_UNUSED(thorns)
    Q_UNUSED(ball)
    
    // 在这里可以添加额外的荆棘碰撞处理逻辑
    qDebug() << "Thorns collision handled";
}

void GameManager::clearAllBalls()
{
    // 清理所有球
    for (BaseBall* ball : m_allBalls) {
        if (ball) {
            removeFromScene(ball);
            ball->deleteLater();
        }
    }
    
    m_allBalls.clear();
    m_players.clear();
    m_foodBalls.clear();
    m_sporeBalls.clear();
    m_thornsBalls.clear();
}

void GameManager::removeFromScene(BaseBall* ball)
{
    if (ball && m_scene && m_scene->items().contains(ball)) {
        m_scene->removeItem(ball);
    }
}
