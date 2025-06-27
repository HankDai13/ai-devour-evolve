#include "GameManager.h"
#include "CloneBall.h"
#include "FoodBall.h"
#include "SporeBall.h"
#include "ThornsBall.h"
#include "GoBiggerConfig.h"
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
    , m_foodRefreshFrameCount(0)
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
        
        // GoBigger风格初始化：生成初始数量的食物
        for (int i = 0; i < m_config.initFoodCount; ++i) {
            QPointF pos = generateRandomFoodPosition();
            FoodBall* food = new FoodBall(getNextBallId(), pos, m_config.gameBorder);
            addBall(food);
        }
        
        // 荆棘初始化保持不变
        for (int i = 0; i < m_config.maxThornsCount / 2; ++i) {
            spawnThorns();
        }
        
        emit gameStarted();
        qDebug() << "Game started with" << m_config.initFoodCount << "initial food balls";
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
    m_foodRefreshFrameCount = 0;  // 重置食物刷新计数器
    
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

// ============ 视野优化方法实现 ============

QVector<BaseBall*> GameManager::getBallsInRect(const QRectF& rect) const
{
    QVector<BaseBall*> ballsInRect;
    
    // 遍历所有球，只返回在指定矩形区域内的球
    for (auto it = m_allBalls.constBegin(); it != m_allBalls.constEnd(); ++it) {
        BaseBall* ball = it.value();
        if (ball && !ball->isRemoved()) {
            QPointF ballPos = ball->pos();
            // 考虑球的半径，使用包含球心+半径的检查
            QRectF ballRect(ballPos.x() - ball->radius(), ballPos.y() - ball->radius(),
                           2 * ball->radius(), 2 * ball->radius());
            
            if (rect.intersects(ballRect)) {
                ballsInRect.append(ball);
            }
        }
    }
    
    return ballsInRect;
}

QVector<FoodBall*> GameManager::getFoodBallsInRect(const QRectF& rect) const
{
    QVector<FoodBall*> foodInRect;
    
    // 只遍历食物球，提升性能
    for (FoodBall* food : m_foodBalls) {
        if (food && !food->isRemoved()) {
            QPointF foodPos = food->pos();
            // 食物球通常较小，可以简化检查
            if (rect.contains(foodPos)) {
                foodInRect.append(food);
            }
        }
    }
    
    return foodInRect;
}

void GameManager::initializeTimers()
{
    // 游戏更新定时器
    m_gameTimer = new QTimer(this);
    connect(m_gameTimer, &QTimer::timeout, this, &GameManager::updateGame);
    m_gameTimer->setInterval(m_config.gameUpdateInterval);
    
    // 食物生成定时器 (GoBigger风格：基于帧数而非固定时间)
    m_foodTimer = new QTimer(this);
    connect(m_foodTimer, &QTimer::timeout, this, &GameManager::spawnFood);
    m_foodTimer->setInterval(m_config.gameUpdateInterval); // 与游戏更新同频，在spawnFood内部按帧数控制
    
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
    
    // 更新所有球的物理状态
    qreal deltaTime = 1.0 / 60.0; // 60 FPS
    
    for (BaseBall* ball : m_allBalls) {
        if (ball && !ball->isRemoved()) {
            // 让每个球自己更新移动（对于孢子球很重要）
            if (ball->ballType() == BaseBall::SPORE_BALL) {
                SporeBall* spore = static_cast<SporeBall*>(ball);
                spore->move(QVector2D(0, 0), deltaTime); // 孢子使用自己的移动逻辑
            }
            // 其他类型的球通过physics自动更新
        }
    }
    
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
    // GoBigger风格的食物补充机制
    m_foodRefreshFrameCount++;
    
    // 每隔指定帧数进行一次补充
    if (m_foodRefreshFrameCount >= m_config.foodRefreshFrames) {
        int currentFoodCount = m_foodBalls.size();
        int leftNum = m_config.maxFoodCount - currentFoodCount;
        
        if (leftNum > 0) {
            // 计算需要补充的食物数量（GoBigger原版公式）
            int todoNum = std::min(
                static_cast<int>(std::ceil(m_config.foodRefreshPercent * leftNum)), 
                leftNum
            );
            
            // 批量生成食物，无需复杂的密度检查
            for (int i = 0; i < todoNum; ++i) {
                QPointF pos = generateRandomFoodPosition();
                FoodBall* food = new FoodBall(getNextBallId(), pos, m_config.gameBorder);
                addBall(food);
            }
            
            if (todoNum > 0) {
                qDebug() << "Spawned" << todoNum << "food balls, total:" << (currentFoodCount + todoNum);
            }
        }
        
        // 重置计数器
        m_foodRefreshFrameCount = 0;
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
    
    // 分别处理不同类型的碰撞，优先处理孢子碰撞
    QVector<SporeBall*> sporesToCheck;
    QVector<CloneBall*> playersToCheck;
    
    // 分类球体
    for (BaseBall* ball : allBalls) {
        if (!ball || ball->isRemoved()) continue;
        
        if (ball->ballType() == BaseBall::SPORE_BALL) {
            sporesToCheck.append(static_cast<SporeBall*>(ball));
        } else if (ball->ballType() == BaseBall::CLONE_BALL) {
            playersToCheck.append(static_cast<CloneBall*>(ball));
        }
    }
    
    // 专门处理玩家球与孢子的碰撞（允许一个玩家球吃多个孢子）
    for (CloneBall* player : playersToCheck) {
        if (!player || player->isRemoved()) continue;
        
        QVector<SporeBall*> sporesToEat;
        for (SporeBall* spore : sporesToCheck) {
            if (!spore || spore->isRemoved()) continue;
            
            // 检查孢子是否可以被吞噬（避免刚生成就被吞噬）
            if (spore->canBeEaten() && player->collidesWith(spore) && player->canEat(spore)) {
                sporesToEat.append(spore);
            }
        }
        
        // 一次性吃掉所有可以吃的孢子
        for (SporeBall* spore : sporesToEat) {
            if (!spore->isRemoved()) {
                qDebug() << "Player" << player->ballId() << "eating spore" << spore->ballId();
                player->eat(spore);
            }
        }
    }
    
    // 然后处理其他类型的碰撞
    for (int i = 0; i < allBalls.size(); ++i) {
        for (int j = i + 1; j < allBalls.size(); ++j) {
            BaseBall* ball1 = allBalls[i];
            BaseBall* ball2 = allBalls[j];
            
            if (!ball1 || !ball2 || ball1->isRemoved() || ball2->isRemoved()) {
                continue;
            }
            
            // 跳过已经处理过的孢子碰撞
            if ((ball1->ballType() == BaseBall::CLONE_BALL && ball2->ballType() == BaseBall::SPORE_BALL) ||
                (ball1->ballType() == BaseBall::SPORE_BALL && ball2->ballType() == BaseBall::CLONE_BALL)) {
                continue;
            }
            
            if (ball1->collidesWith(ball2)) {
                checkCollisionsBetween(ball1, ball2);
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
                 << "Player score:" << player->score()
                 << "Food score:" << food->score();
        
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
        
        qDebug() << "Player-Spore collision detected!" 
                 << "Player radius:" << player->radius() 
                 << "Spore radius:" << spore->radius()
                 << "Player score:" << player->score()
                 << "Spore score:" << spore->score()
                 << "Player team:" << player->teamId()
                 << "Spore team:" << spore->teamId()
                 << "Distance:" << player->distanceTo(spore);
        
        // 孢子球可以被任何玩家球吞噬（包括自己的），符合GoBigger原版
        if (player->canEat(spore)) {
            qDebug() << "Player CAN eat spore - eating now...";
            player->eat(spore);
        } else {
            qDebug() << "Player CANNOT eat spore - score ratio too small"
                     << "Required ratio:" << GoBiggerConfig::EAT_RATIO
                     << "Actual ratio:" << (player->score() / spore->score());
        }
    }
    
    // 玩家球 vs 玩家球
    else if (ball1->ballType() == BaseBall::CLONE_BALL && ball2->ballType() == BaseBall::CLONE_BALL) {
        CloneBall* player1 = static_cast<CloneBall*>(ball1);
        CloneBall* player2 = static_cast<CloneBall*>(ball2);
        
        // 同队玩家球的处理
        if (player1->teamId() == player2->teamId() && player1->playerId() == player2->playerId()) {
            // 检查是否需要刚体碰撞
            if (player1->shouldRigidCollide(player2)) {
                player1->rigidCollision(player2);
            }
        }
        // 不同队伍的玩家球可以互相吞噬
        else if (player1->teamId() != player2->teamId()) {
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
