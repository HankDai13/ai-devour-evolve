#include "GameManager.h"
#include "CloneBall.h"
#include "FoodBall.h"
#include "SporeBall.h"
#include "ThornsBall.h"
#include "GoBiggerConfig.h"
#include "QuadTree.h"
#include "SimpleAIPlayer.h"
#include <QGraphicsScene>
#include <QDebug>
#include <cmath>
#include <memory>

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
    , m_thornsRefreshFrameCount(0)
    , m_defaultAIModelPath("assets/ai_models/exported_models/ai_model_traced.pt")
{
    // 初始化四叉树 - 使用游戏边界
    QRectF bounds(m_config.gameBorder.minx, m_config.gameBorder.miny,
                  m_config.gameBorder.maxx - m_config.gameBorder.minx,
                  m_config.gameBorder.maxy - m_config.gameBorder.miny);
    m_quadTree = std::make_unique<QuadTree>(bounds, 6, 8); // 最大深度6，每节点最多8个球
    
    initializeTimers();
}

GameManager::~GameManager()
{
    // 清理AI玩家
    for (auto aiPlayer : m_aiPlayers) {
        if (aiPlayer) {
            aiPlayer->stopAI();
            delete aiPlayer;
        }
    }
    m_aiPlayers.clear();
    
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
        
        // GoBigger风格初始化：生成初始数量的荆棘球
        for (int i = 0; i < m_config.initThornsCount; ++i) {
            QPointF pos = generateRandomThornsPosition();
            int score = m_config.thornsScoreMin + 
                       QRandomGenerator::global()->bounded(m_config.thornsScoreMax - m_config.thornsScoreMin + 1);
            ThornsBall* thorns = new ThornsBall(getNextBallId(), pos, m_config.gameBorder);
            thorns->setScore(score);
            addBall(thorns);
            qDebug() << "Created thorns ball" << thorns->ballId() << "at" << pos << "with score" << score;
        }
        
        emit gameStarted();
        qDebug() << "Game started with" << m_config.initFoodCount << "initial food balls and" << m_config.initThornsCount << "initial thorns balls";
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
    m_thornsRefreshFrameCount = 0;  // 重置荆棘刷新计数器
    
    emit gameReset();
    qDebug() << "Game reset";
}

CloneBall* GameManager::createPlayer(int teamId, int playerId, const QPointF& position)
{
    qDebug() << "🔨 createPlayer called: teamId=" << teamId << "playerId=" << playerId;
    
    // 🔥 修复：更严格的重复创建检查，防止人类玩家被重复创建
    for (CloneBall* player : m_players) {
        if (player && !player->isRemoved() && 
            player->teamId() == teamId && player->playerId() == playerId) {
            qDebug() << "🔨 Player already exists:" << teamId << playerId << "- returning existing player";
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
    connect(player, &CloneBall::thornsEaten, this, &GameManager::handleThornsEaten); // 🔥 添加荆棘球信号连接
    
    emit playerAdded(player);
    qDebug() << "🔨 Player created: teamId=" << teamId << "playerId=" << playerId 
             << "ballId=" << player->ballId() << "at" << spawnPos;
    
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
    
    // 荆棘生成定时器 (GoBigger风格：基于帧数)
    m_thornsTimer = new QTimer(this);
    connect(m_thornsTimer, &QTimer::timeout, this, &GameManager::spawnThorns);
    m_thornsTimer->setInterval(m_config.gameUpdateInterval); // 与游戏更新同频，在spawnThorns内部按帧数控制
}

void GameManager::connectBallSignals(BaseBall* ball)
{
    if (!ball) return;
    
    connect(ball, &BaseBall::ballRemoved, this, &GameManager::handleBallRemoved);
    
    // 连接特定类型的信号
    if (ball->ballType() == BaseBall::THORNS_BALL) {
        ThornsBall* thorns = static_cast<ThornsBall*>(ball);
        connect(thorns, &ThornsBall::thornsCollision, this, &GameManager::handleThornsCollision);
    } else if (ball->ballType() == BaseBall::CLONE_BALL) {
        CloneBall* clone = static_cast<CloneBall*>(ball);
        connect(clone, &CloneBall::splitPerformed, this, &GameManager::handlePlayerSplit);
        connect(clone, &CloneBall::sporeEjected, this, &GameManager::handleSporeEjected);
        connect(clone, &CloneBall::thornsEaten, this, &GameManager::handleThornsEaten);
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
            // 让每个球自己更新移动（对于孢子球和荆棘球很重要）
            if (ball->ballType() == BaseBall::SPORE_BALL) {
                SporeBall* spore = static_cast<SporeBall*>(ball);
                spore->move(QVector2D(0, 0), deltaTime); // 孢子使用自己的移动逻辑
            }
            // 🔥 荆棘球也需要更新移动状态（吃孢子后的滑行）
            else if (ball->ballType() == BaseBall::THORNS_BALL) {
                ThornsBall* thorns = static_cast<ThornsBall*>(ball);
                thorns->move(QVector2D(0, 0), deltaTime); // 荆棘球更新移动状态
            }
            // 其他类型的球通过physics自动更新
        }
    }
    
    // 检查碰撞 - 使用GoBigger优化算法
    checkCollisionsOptimized();
    
    // 额外的同玩家分身球合并检查 - 解决复杂分裂后的合并问题
    QSet<QPair<int, int>> checkedPlayers;
    for (CloneBall* player : m_players) {
        if (player && !player->isRemoved()) {
            QPair<int, int> playerKey(player->teamId(), player->playerId());
            if (!checkedPlayers.contains(playerKey)) {
                checkPlayerBallsMerging(player->teamId(), player->playerId());
                checkedPlayers.insert(playerKey);
            }
        }
    }
    
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

    // Check for game over
    QSet<int> activeTeams;
    for (CloneBall* player : m_players) {
        if (player && !player->isRemoved()) {
            activeTeams.insert(player->teamId());
        }
    }

    if (activeTeams.size() <= 1) {
        int winningTeamId = -1;
        if (activeTeams.size() == 1) {
            winningTeamId = *activeTeams.begin();
        }
        emit gameOver(winningTeamId);
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
    // GoBigger风格的荆棘补充机制
    m_thornsRefreshFrameCount++;
    
    // 每隔指定帧数进行一次补充
    if (m_thornsRefreshFrameCount >= m_config.thornsRefreshFrames) {
        int currentThornsCount = m_thornsBalls.size();
        int leftNum = m_config.maxThornsCount - currentThornsCount;
        
        if (leftNum > 0) {
            // 计算需要补充的荆棘数量（GoBigger原版公式）
            int todoNum = std::min(
                static_cast<int>(std::ceil(m_config.thornsRefreshPercent * leftNum)), 
                leftNum
            );
            
            // 批量生成荆棘球
            for (int i = 0; i < todoNum; ++i) {
                QPointF pos = generateRandomThornsPosition();
                // 使用GoBigger标准的分数范围
                int score = m_config.thornsScoreMin + 
                           QRandomGenerator::global()->bounded(m_config.thornsScoreMax - m_config.thornsScoreMin + 1);
                ThornsBall* thorns = new ThornsBall(getNextBallId(), pos, m_config.gameBorder);
                thorns->setScore(score);
                addBall(thorns);
            }
            
            if (todoNum > 0) {
                qDebug() << "Spawned" << todoNum << "thorns balls, total:" << (currentThornsCount + todoNum);
            }
        }
        
        // 重置计数器
        m_thornsRefreshFrameCount = 0;
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
    
    // 荆棘球 vs 孢子球 (GoBigger特殊机制)
    else if ((ball1->ballType() == BaseBall::THORNS_BALL && ball2->ballType() == BaseBall::SPORE_BALL) ||
             (ball1->ballType() == BaseBall::SPORE_BALL && ball2->ballType() == BaseBall::THORNS_BALL)) {
        
        ThornsBall* thorns = (ball1->ballType() == BaseBall::THORNS_BALL) ? 
                            static_cast<ThornsBall*>(ball1) : static_cast<ThornsBall*>(ball2);
        SporeBall* spore = (ball1->ballType() == BaseBall::SPORE_BALL) ? 
                          static_cast<SporeBall*>(ball1) : static_cast<SporeBall*>(ball2);
        
        qDebug() << "Thorns-Spore collision detected! Thorns" << thorns->ballId() 
                 << "eating spore" << spore->ballId();
        
        // 荆棘球吃孢子，获得移动能力
        thorns->eatSpore(spore);
    }
    
    // 玩家球 vs 荆棘球 (GoBigger特殊机制：玩家可以吃荆棘)
    else if ((ball1->ballType() == BaseBall::CLONE_BALL && ball2->ballType() == BaseBall::THORNS_BALL) ||
             (ball1->ballType() == BaseBall::THORNS_BALL && ball2->ballType() == BaseBall::CLONE_BALL)) {
        
        CloneBall* player = (ball1->ballType() == BaseBall::CLONE_BALL) ? 
                           static_cast<CloneBall*>(ball1) : static_cast<CloneBall*>(ball2);
        ThornsBall* thorns = (ball1->ballType() == BaseBall::THORNS_BALL) ? 
                            static_cast<ThornsBall*>(ball1) : static_cast<ThornsBall*>(ball2);
        
        // GoBigger机制：玩家可以吃荆棘球，触发特殊分裂
        if (player->canEat(thorns)) {
            qDebug() << "Player" << player->ballId() << "eating thorns" << thorns->ballId() 
                     << "- will trigger special split";
            
            // 需要传递当前玩家的总球数，这里先用队友总数估算
            int totalPlayerBalls = 0;
            for (CloneBall* p : m_players) {
                if (p && !p->isRemoved() && p->teamId() == player->teamId() && p->playerId() == player->playerId()) {
                    totalPlayerBalls++;
                }
            }
            
            // 先吃荆棘球 - eat方法内部会调用performThornsSplit
            player->eat(thorns);
        } else {
            // 如果不能吃，则荆棘球造成伤害
            thorns->causeCollisionDamage(player);
        }
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
    qDebug() << "🔄 handlePlayerSplit: originalBall=" << originalBall->ballId() 
             << "teamId=" << originalBall->teamId() << "playerId=" << originalBall->playerId()
             << "newBalls count=" << newBalls.size();
    
    // 将新分裂的球添加到管理器
    for (CloneBall* newBall : newBalls) {
        addBall(newBall);
        m_players.append(newBall);
        
        // 连接新球的信号
        connect(newBall, &CloneBall::splitPerformed, this, &GameManager::handlePlayerSplit);
        connect(newBall, &CloneBall::sporeEjected, this, &GameManager::handleSporeEjected);
        
        qDebug() << "🔄 Added split ball: ballId=" << newBall->ballId() 
                 << "teamId=" << newBall->teamId() << "playerId=" << newBall->playerId();
    }
    
    qDebug() << "🔄 Player split complete. Total players now:" << m_players.size();
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
    
    // 荆棘碰撞不产生任何影响（GoBigger标准：只有能吃才有效果）
    qDebug() << "Thorns collision - no effect";
}

void GameManager::handleThornsEaten(CloneBall* ball, ThornsBall* thorns)
{
    if (!ball || !thorns) return;
    
    qDebug() << "GameManager: Player" << ball->ballId() << "ate thorns" << thorns->ballId();
    
    // 计算当前玩家的总球数
    int totalPlayerBalls = 0;
    for (CloneBall* p : m_players) {
        if (p && !p->isRemoved() && p->teamId() == ball->teamId() && p->playerId() == ball->playerId()) {
            totalPlayerBalls++;
        }
    }
    
    qDebug() << "Player has" << totalPlayerBalls << "total balls before thorns split";
    
    // 执行荆棘分裂
    QVector<CloneBall*> newBalls = ball->performThornsSplit(QVector2D(1, 0), totalPlayerBalls);
    
    // 添加新球到管理器
    for (CloneBall* newBall : newBalls) {
        addBall(newBall);
        m_players.append(newBall);
        
        // 连接新球的信号
        connectBallSignals(newBall);
    }
    
    qDebug() << "Thorns split completed: created" << newBalls.size() << "new balls";
    
    // 发送分裂信号
    if (!newBalls.isEmpty()) {
        emit playerAdded(ball); // 通知有新的玩家球
    }
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

// ============ GoBigger优化碰撞检测实现 ============

void GameManager::checkCollisionsOptimized()
{
    // 重建四叉树 - 每帧重建以确保数据最新
    QVector<BaseBall*> allBalls = getAllBalls();
    m_quadTree->rebuild(allBalls);
    
    // GoBigger优化策略1: 只检测移动的球体
    QVector<BaseBall*> movingBalls = getMovingBalls();
    
    // 性能统计
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 60 == 0) { // 每60帧输出一次统计
        qDebug() << "Collision optimization stats:"
                 << "Total balls:" << allBalls.size()
                 << "Moving balls:" << movingBalls.size()
                 << "QuadTree nodes:" << m_quadTree->getNodeCount()
                 << "QuadTree depth:" << m_quadTree->getMaxDepth();
    }
    
    // 对每个移动的球体，使用四叉树查找可能碰撞的候选者
    for (BaseBall* movingBall : movingBalls) {
        if (!movingBall || movingBall->isRemoved()) continue;
        
        // 使用四叉树查询可能碰撞的球体
        QVector<BaseBall*> candidates = m_quadTree->queryCollisions(movingBall);
        
        // 检查与候选者的碰撞
        for (BaseBall* candidate : candidates) {
            if (!candidate || candidate->isRemoved()) continue;
            if (candidate == movingBall) continue; // 跳过自己
            
            if (movingBall->collidesWith(candidate)) {
                checkCollisionsBetween(movingBall, candidate);
            }
        }
    }
    
    // 特殊处理：孢子与玩家球的优化碰撞检测
    // 这是GoBigger的一个关键优化：允许一个玩家球在一帧内吃多个孢子
    optimizeSporeCollisions();
}

QVector<BaseBall*> GameManager::getMovingBalls() const
{
    QVector<BaseBall*> movingBalls;
    
    // 玩家球总是被认为是移动的（即使静止，也可能随时移动）
    for (CloneBall* player : m_players) {
        if (player && !player->isRemoved()) {
            movingBalls.append(player);
        }
    }
    
    // 孢子球总是移动的
    for (SporeBall* spore : m_sporeBalls) {
        if (spore && !spore->isRemoved()) {
            movingBalls.append(spore);
        }
    }
    
    // 荆棘球：所有荆棘球都参与碰撞检测（无论是否移动）
    // 因为静止的荆棘球也需要检测与孢子的碰撞
    for (ThornsBall* thorns : m_thornsBalls) {
        if (thorns && !thorns->isRemoved()) {
            movingBalls.append(thorns);
        }
    }
    
    // 食物球通常是静止的，不包含在移动球列表中
    // 这是GoBigger优化的核心：大量静态食物不参与主动碰撞检测
    
    return movingBalls;
}

void GameManager::optimizeSporeCollisions()
{
    // 针对孢子的特殊优化：允许玩家球在一帧内吃掉多个孢子
    for (CloneBall* player : m_players) {
        if (!player || player->isRemoved()) continue;
        
        QVector<SporeBall*> sporesToEat;
        
        // 使用四叉树查找附近的孢子
        QVector<BaseBall*> candidates = m_quadTree->queryCollisions(player);
        
        for (BaseBall* candidate : candidates) {
            if (!candidate || candidate->isRemoved()) continue;
            if (candidate->ballType() != BaseBall::SPORE_BALL) continue;
            
            SporeBall* spore = static_cast<SporeBall*>(candidate);
            if (spore->canBeEaten() && player->collidesWith(spore) && player->canEat(spore)) {
                sporesToEat.append(spore);
            }
        }
        
        // 一次性吃掉所有可以吃的孢子
        for (SporeBall* spore : sporesToEat) {
            if (!spore->isRemoved()) {
                player->eat(spore);
            }
        }
    }
}

void GameManager::checkPlayerBallsMerging(int teamId, int playerId)
{
    QVector<CloneBall*> playerBalls = getPlayerBalls(teamId, playerId);
    
    // 对每个球检查与其他球的合并可能性
    for (int i = 0; i < playerBalls.size(); ++i) {
        CloneBall* ball1 = playerBalls[i];
        if (!ball1 || ball1->isRemoved()) continue;
        
        for (int j = i + 1; j < playerBalls.size(); ++j) {
            CloneBall* ball2 = playerBalls[j];
            if (!ball2 || ball2->isRemoved()) continue;
            
            // 检查两球是否可以合并
            if (ball1->canMergeWith(ball2)) {
                qDebug() << "GameManager: Auto-merging balls" << ball1->ballId() << "and" << ball2->ballId();
                ball1->mergeWith(ball2);
                return; // 一次只合并一对球
            }
        }
    }
}

QVector<CloneBall*> GameManager::getPlayerBalls(int teamId, int playerId) const
{
    QVector<CloneBall*> playerBalls;
    
    for (CloneBall* player : m_players) {
        if (player && !player->isRemoved() && 
            player->teamId() == teamId && player->playerId() == playerId) {
            playerBalls.append(player);
        }
    }
    
    return playerBalls;
}

// AI玩家管理实现
bool GameManager::addAIPlayer(int teamId, int playerId, const QString& aiModelPath)
{
    // 检查是否已经存在相同的AI玩家
    for (auto aiPlayer : m_aiPlayers) {
        if (aiPlayer && aiPlayer->getPlayerBall() && 
            aiPlayer->getPlayerBall()->teamId() == teamId && 
            aiPlayer->getPlayerBall()->playerId() == playerId) {
            qWarning() << "AI player already exists for team" << teamId << "player" << playerId;
            return false;
        }
    }
    
    // 创建新的CloneBall作为AI玩家
    QPointF startPos = generateRandomPosition();
    CloneBall* playerBall = new CloneBall(
        getNextBallId(),
        startPos,
        m_config.gameBorder,
        teamId,
        playerId
    );
    
    // 添加到游戏中
    addBall(playerBall);
    m_players.append(playerBall);  // 重要：添加到玩家列表中
    
    // 连接玩家特有的信号
    connect(playerBall, &CloneBall::splitPerformed, this, &GameManager::handlePlayerSplit);
    connect(playerBall, &CloneBall::sporeEjected, this, &GameManager::handleSporeEjected);
    connect(playerBall, &CloneBall::thornsEaten, this, &GameManager::handleThornsEaten);
    
    // 发出玩家添加信号
    emit playerAdded(playerBall);
    
    // 创建AI控制器
    auto aiPlayer = new GoBigger::AI::SimpleAIPlayer(playerBall, this);
    
    // 加载AI模型
    if (!aiModelPath.isEmpty()) {
        if (!aiPlayer->loadAIModel(aiModelPath)) {
            qWarning() << "Failed to load AI model from" << aiModelPath << "for player" << teamId << playerId;
            qWarning() << "Using default heuristic strategy instead";
            // 根据teamId设置不同的默认策略
            if (teamId == 2) {
                aiPlayer->setAIStrategy(GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE);
                qDebug() << "Set aggressive strategy for RL-AI fallback";
            } else {
                aiPlayer->setAIStrategy(GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER);
            }
        } else {
            qDebug() << "Successfully loaded AI model from" << aiModelPath;
            // 设置为模型策略
            aiPlayer->setAIStrategy(GoBigger::AI::SimpleAIPlayer::AIStrategy::MODEL_BASED);
        }
    } else {
        qDebug() << "No AI model path specified, using heuristic strategies";
        // 根据teamId使用不同的默认策略
        if (teamId == 1) {
            aiPlayer->setAIStrategy(GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER);
        } else if (teamId == 2) {
            aiPlayer->setAIStrategy(GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE);
        } else {
            aiPlayer->setAIStrategy(GoBigger::AI::SimpleAIPlayer::AIStrategy::RANDOM);
        }
    }
    
    m_aiPlayers.append(aiPlayer);
    
    // 连接AI销毁信号
    connect(aiPlayer, &GoBigger::AI::SimpleAIPlayer::aiPlayerDestroyed,
            this, &GameManager::handleAIPlayerDestroyed);
    
    // 启动AI控制
    aiPlayer->startAI();
    
    qDebug() << "Successfully added AI player for team" << teamId << "player" << playerId 
             << "at position" << startPos << "and started AI";
    
    return true;
}

bool GameManager::addAIPlayerWithStrategy(int teamId, int playerId, 
                                          GoBigger::AI::AIStrategy strategy,
                                          const QString& aiModelPath)
{
    // 检查是否已经存在相同的AI玩家
    for (auto aiPlayer : m_aiPlayers) {
        if (aiPlayer && aiPlayer->getPlayerBall() && 
            aiPlayer->getPlayerBall()->teamId() == teamId && 
            aiPlayer->getPlayerBall()->playerId() == playerId) {
            qWarning() << "AI player already exists for team" << teamId << "player" << playerId;
            return false;
        }
    }
    
    // 创建新的CloneBall作为AI玩家
    QPointF startPos = generateRandomPosition();
    CloneBall* playerBall = new CloneBall(
        getNextBallId(),
        startPos,
        m_config.gameBorder,
        teamId,
        playerId
    );
    
    // 添加到游戏中
    addBall(playerBall);
    m_players.append(playerBall);  // 重要：添加到玩家列表中
    
    qDebug() << "Created CloneBall for AI:" 
             << "teamId=" << teamId 
             << "playerId=" << playerId 
             << "position=" << startPos 
             << "ballId=" << playerBall->ballId()
             << "in scene=" << (playerBall->scene() != nullptr);
    
    // 连接玩家特有的信号
    connect(playerBall, &CloneBall::splitPerformed, this, &GameManager::handlePlayerSplit);
    connect(playerBall, &CloneBall::sporeEjected, this, &GameManager::handleSporeEjected);
    connect(playerBall, &CloneBall::thornsEaten, this, &GameManager::handleThornsEaten);
    
    // 发出玩家添加信号
    emit playerAdded(playerBall);
    
    // 创建AI控制器
    auto aiPlayer = new GoBigger::AI::SimpleAIPlayer(playerBall, this);
    
    // 转换策略类型 - 从前置声明转换到实际枚举
    GoBigger::AI::SimpleAIPlayer::AIStrategy actualStrategy;
    switch (strategy) {
        case GoBigger::AI::AIStrategy::RANDOM:
            actualStrategy = GoBigger::AI::SimpleAIPlayer::AIStrategy::RANDOM;
            break;
        case GoBigger::AI::AIStrategy::FOOD_HUNTER:
            actualStrategy = GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER;
            break;
        case GoBigger::AI::AIStrategy::AGGRESSIVE:
            actualStrategy = GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE;
            break;
        case GoBigger::AI::AIStrategy::MODEL_BASED:
            actualStrategy = GoBigger::AI::SimpleAIPlayer::AIStrategy::MODEL_BASED;
            break;
        default:
            actualStrategy = GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER;
            break;
    }
    
    // 首先设置指定的策略
    aiPlayer->setAIStrategy(actualStrategy);
    
    // 如果是MODEL_BASED策略且提供了模型路径，尝试加载模型
    if (actualStrategy == GoBigger::AI::SimpleAIPlayer::AIStrategy::MODEL_BASED && !aiModelPath.isEmpty()) {
        if (!aiPlayer->loadAIModel(aiModelPath)) {
            qWarning() << "Failed to load AI model from" << aiModelPath << "for player" << teamId << playerId;
            // 根据teamId选择合适的回退策略
            if (teamId == 2) {
                qWarning() << "RL-AI model load failed, using AGGRESSIVE strategy as fallback";
                aiPlayer->setAIStrategy(GoBigger::AI::SimpleAIPlayer::AIStrategy::AGGRESSIVE);
            } else {
                qWarning() << "Custom AI model load failed, using FOOD_HUNTER strategy as fallback";
                aiPlayer->setAIStrategy(GoBigger::AI::SimpleAIPlayer::AIStrategy::FOOD_HUNTER);
            }
        } else {
            qDebug() << "Successfully loaded AI model from" << aiModelPath;
        }
    }
    
    m_aiPlayers.append(aiPlayer);
    
    // 连接AI销毁信号
    connect(aiPlayer, &GoBigger::AI::SimpleAIPlayer::aiPlayerDestroyed,
            this, &GameManager::handleAIPlayerDestroyed);
    
    // 启动AI控制
    aiPlayer->startAI();
    
    qDebug() << "Successfully added AI player for team" << teamId << "player" << playerId 
             << "at position" << startPos << "with strategy" << static_cast<int>(strategy) << "and started AI";
    
    return true;
}

void GameManager::removeAIPlayer(int teamId, int playerId)
{
    for (int i = 0; i < m_aiPlayers.size(); ++i) {
        auto aiPlayer = m_aiPlayers[i];
        if (aiPlayer && aiPlayer->getPlayerBall() && 
            aiPlayer->getPlayerBall()->teamId() == teamId && 
            aiPlayer->getPlayerBall()->playerId() == playerId) {
            
            aiPlayer->stopAI();
            CloneBall* playerBall = aiPlayer->getPlayerBall();
            if (playerBall) {
                m_players.removeOne(playerBall);  // 从玩家列表中移除
                emit playerRemoved(playerBall);   // 发出移除信号
                removeBall(playerBall);
            }
            
            m_aiPlayers.removeAt(i);
            delete aiPlayer;
            
            qDebug() << "Removed AI player for team" << teamId << "player" << playerId;
            return;
        }
    }
    
    qWarning() << "AI player not found for team" << teamId << "player" << playerId;
}

void GameManager::startAllAI()
{
    for (auto aiPlayer : m_aiPlayers) {
        if (aiPlayer) {
            aiPlayer->startAI();
        }
    }
    qDebug() << "Started" << m_aiPlayers.size() << "AI players";
}

void GameManager::stopAllAI()
{
    for (auto aiPlayer : m_aiPlayers) {
        if (aiPlayer) {
            aiPlayer->stopAI();
        }
    }
    qDebug() << "Stopped" << m_aiPlayers.size() << "AI players";
}

void GameManager::removeAllAI()
{
    // 停止所有AI
    stopAllAI();
    
    // 移除所有AI球体
    for (auto aiPlayer : m_aiPlayers) {
        if (aiPlayer && aiPlayer->getPlayerBall()) {
            CloneBall* playerBall = aiPlayer->getPlayerBall();
            m_players.removeOne(playerBall);  // 从玩家列表中移除
            removeBall(playerBall);
        }
        delete aiPlayer;
    }
    
    m_aiPlayers.clear();
    qDebug() << "Removed all AI players";
}

void GameManager::handleAIPlayerDestroyed(GoBigger::AI::SimpleAIPlayer* aiPlayer)
{
    if (!aiPlayer) return;
    
    qDebug() << "AI player destroyed, removing from manager";
    
    // 从AI玩家列表中移除
    m_aiPlayers.removeOne(aiPlayer);
    
    // 不需要手动删除aiPlayer，因为它会自动销毁
    // 但我们需要从调试台中移除它（如果有的话）
    // 这将通过信号机制由GameView处理
    
    qDebug() << "AI player removed from manager, remaining AI count:" << m_aiPlayers.size();
}
