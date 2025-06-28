#include "GameEngine.h"
#include "data/BaseBallData.h"
#include "data/CloneBallData.h"
#include "data/FoodBallData.h"
#include "data/SporeBallData.h"
#include "data/ThornsBallData.h"
#include "CoreUtils.h"
#include "../GoBiggerConfig.h"
#include <QTimer>
#include <QDebug>
#include <cmath>
#include <algorithm>

GameEngine::GameEngine(const Config& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_gameRunning(false)
    , m_totalFrames(0)
    , m_nextBallId(1)
    , m_foodRefreshFrameCount(0)
    , m_thornsRefreshFrameCount(0)
{
    // 初始化四叉树
    QRectF bounds(m_config.gameBorder.minx, m_config.gameBorder.miny,
                  m_config.gameBorder.maxx - m_config.gameBorder.minx,
                  m_config.gameBorder.maxy - m_config.gameBorder.miny);
    m_quadTree = std::make_unique<CoreQuadTree>(bounds, 6, 8);
}

GameEngine::~GameEngine()
{
    clearAllBalls();
}

Observation GameEngine::reset()
{
    resetGame();
    startGame();
    
    // 创建默认玩家（如果没有的话）
    if (m_players.isEmpty()) {
        createPlayer(0, 0, QPointF(0, 0));
    }
    
    return getObservation();
}

Observation GameEngine::step(const Action& action)
{
    // 限制动作范围
    Action clampedAction;
    clampedAction.direction_x = qBound(-1.0f, action.direction_x, 1.0f);
    clampedAction.direction_y = qBound(-1.0f, action.direction_y, 1.0f);
    clampedAction.action_type = qBound(0, action.action_type, 2);
    
    // 应用动作到主玩家（假设是第一个玩家）
    if (!m_players.isEmpty()) {
        CloneBallData* mainPlayer = m_players.first();
        
        // 移动
        if (clampedAction.direction_x != 0 || clampedAction.direction_y != 0) {
            QVector2D direction(clampedAction.direction_x, clampedAction.direction_y);
            mainPlayer->move(direction, 1.0f / 60.0f);
        }
        
        // 特殊动作
        switch (clampedAction.action_type) {
            case 1: // 吐孢子
                if (mainPlayer->canEject()) {
                    QVector2D ejectDir(clampedAction.direction_x, clampedAction.direction_y);
                    if (ejectDir.length() == 0) ejectDir = QVector2D(1, 0);
                    
                    SporeBallData* spore = mainPlayer->ejectSpore(ejectDir);
                    if (spore) {
                        spore->setBallId(getNextBallId()); // 设置ID
                        addBall(spore);
                    }
                }
                break;
                
            case 2: // 分裂
                if (mainPlayer->canSplit()) {
                    QVector2D splitDir(clampedAction.direction_x, clampedAction.direction_y);
                    if (splitDir.length() == 0) splitDir = QVector2D(1, 0);
                    
                    QVector<CloneBallData*> newBalls = mainPlayer->performSplit(splitDir);
                    for (CloneBallData* newBall : newBalls) {
                        newBall->setBallId(getNextBallId()); // 设置ID
                        addBall(newBall);
                        m_players.append(newBall);
                    }
                }
                break;
        }
    }
    
    // 更新游戏状态
    updateGame();
    
    return getObservation();
}

bool GameEngine::isDone() const
{
    // 游戏结束条件：所有玩家球都被移除
    for (CloneBallData* player : m_players) {
        if (player && !player->isRemoved()) {
            return false;
        }
    }
    return true;
}

void GameEngine::startGame()
{
    if (!m_gameRunning) {
        m_gameRunning = true;
        m_totalFrames = 0;
        
        // 生成初始食物
        for (int i = 0; i < m_config.initFoodCount; ++i) {
            QPointF pos = generateRandomFoodPosition();
            FoodBallData* food = new FoodBallData(getNextBallId(), pos, 
                BaseBallData::Border(m_config.gameBorder.minx, m_config.gameBorder.maxx,
                                   m_config.gameBorder.miny, m_config.gameBorder.maxy));
            addBall(food);
        }
        
        // 生成初始荆棘球
        for (int i = 0; i < m_config.initThornsCount; ++i) {
            QPointF pos = generateRandomThornsPosition();
            ThornsBallData* thorns = new ThornsBallData(getNextBallId(), pos,
                BaseBallData::Border(m_config.gameBorder.minx, m_config.gameBorder.maxx,
                                   m_config.gameBorder.miny, m_config.gameBorder.maxy));
            int score = m_config.thornsScoreMin + 
                       QRandomGenerator::global()->bounded(m_config.thornsScoreMax - m_config.thornsScoreMin + 1);
            thorns->setScore(score);
            addBall(thorns);
        }
        
        emit gameStarted();
        qDebug() << "GameEngine started with" << m_config.initFoodCount << "food and" << m_config.initThornsCount << "thorns";
    }
}

void GameEngine::pauseGame()
{
    if (m_gameRunning) {
        m_gameRunning = false;
        emit gamePaused();
        qDebug() << "GameEngine paused";
    }
}

void GameEngine::resetGame()
{
    pauseGame();
    clearAllBalls();
    m_nextBallId = 1;
    m_totalFrames = 0;
    m_foodRefreshFrameCount = 0;
    m_thornsRefreshFrameCount = 0;
    
    emit gameReset();
    qDebug() << "GameEngine reset";
}

CloneBallData* GameEngine::createPlayer(int teamId, int playerId, const QPointF& position)
{
    // 检查是否已存在相同的玩家
    for (CloneBallData* player : m_players) {
        if (player->teamId() == teamId && player->playerId() == playerId) {
            qDebug() << "Player already exists:" << teamId << playerId;
            return player;
        }
    }
    
    QPointF spawnPos = position.isNull() ? generateRandomPosition() : position;
    
    CloneBallData* player = new CloneBallData(
        getNextBallId(),
        spawnPos,
        BaseBallData::Border(m_config.gameBorder.minx, m_config.gameBorder.maxx,
                           m_config.gameBorder.miny, m_config.gameBorder.maxy),
        teamId,
        playerId
    );
    
    addBall(player);
    m_players.append(player);
    
    qDebug() << "GameEngine: Player created:" << teamId << playerId << "at" << spawnPos;
    
    return player;
}

CloneBallData* GameEngine::getPlayer(int teamId, int playerId) const
{
    for (CloneBallData* player : m_players) {
        if (player->teamId() == teamId && player->playerId() == playerId) {
            return player;
        }
    }
    return nullptr;
}

QVector<BaseBallData*> GameEngine::getAllBalls() const
{
    return m_allBalls.values().toVector();
}

float GameEngine::getTotalPlayerScore(int teamId, int playerId) const
{
    float totalScore = 0.0f;
    
    for (CloneBallData* player : m_players) {
        if (player && !player->isRemoved() && 
            player->teamId() == teamId && player->playerId() == playerId) {
            totalScore += player->score();
        }
    }
    
    return totalScore;
}

void GameEngine::addBall(BaseBallData* ball)
{
    if (!ball) return;
    
    m_allBalls.insert(ball->ballId(), ball);
    
    // 根据类型添加到相应的列表
    switch (ball->ballType()) {
        case BaseBallData::CLONE_BALL:
            // 玩家球已在createPlayer中处理
            break;
        case BaseBallData::FOOD_BALL:
            m_foodBalls.append(static_cast<FoodBallData*>(ball));
            break;
        case BaseBallData::SPORE_BALL:
            m_sporeBalls.append(static_cast<SporeBallData*>(ball));
            break;
        case BaseBallData::THORNS_BALL:
            m_thornsBalls.append(static_cast<ThornsBallData*>(ball));
            break;
    }
    
    emit ballAdded(ball);
}

void GameEngine::removeBall(BaseBallData* ball)
{
    if (!ball) return;
    
    m_allBalls.remove(ball->ballId());
    
    // 从相应的列表中移除
    switch (ball->ballType()) {
        case BaseBallData::CLONE_BALL:
            m_players.removeOne(static_cast<CloneBallData*>(ball));
            break;
        case BaseBallData::FOOD_BALL:
            m_foodBalls.removeOne(static_cast<FoodBallData*>(ball));
            break;
        case BaseBallData::SPORE_BALL:
            m_sporeBalls.removeOne(static_cast<SporeBallData*>(ball));
            break;
        case BaseBallData::THORNS_BALL:
            m_thornsBalls.removeOne(static_cast<ThornsBallData*>(ball));
            break;
    }
    
    emit ballRemoved(ball);
}

void GameEngine::clearAllBalls()
{
    // 清理所有球
    for (BaseBallData* ball : m_allBalls) {
        if (ball) {
            delete ball; // 直接删除，不使用deleteLater
        }
    }
    
    m_allBalls.clear();
    m_players.clear();
    m_foodBalls.clear();
    m_sporeBalls.clear();
    m_thornsBalls.clear();
}

QPointF GameEngine::generateRandomPosition() const
{
    QRandomGenerator* rng = QRandomGenerator::global();
    
    qreal x = m_config.gameBorder.minx + 
              (m_config.gameBorder.maxx - m_config.gameBorder.minx) * rng->generateDouble();
    qreal y = m_config.gameBorder.miny + 
              (m_config.gameBorder.maxy - m_config.gameBorder.miny) * rng->generateDouble();
    
    return QPointF(x, y);
}

QPointF GameEngine::generateRandomFoodPosition() const
{
    return generateRandomPosition();
}

QPointF GameEngine::generateRandomThornsPosition() const
{
    // 荆棘生成位置避开玩家球附近
    QPointF pos;
    int attempts = 0;
    const int maxAttempts = 50;
    
    do {
        pos = generateRandomPosition();
        bool tooClose = false;
        
        for (CloneBallData* player : m_players) {
            if (player && !player->isRemoved()) {
                qreal distance = std::sqrt(std::pow(pos.x() - player->position().x(), 2) + 
                                          std::pow(pos.y() - player->position().y(), 2));
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

void GameEngine::updateGame()
{
    if (!m_gameRunning) return;
    
    m_totalFrames++;
    
    // 更新所有球的物理状态
    qreal deltaTime = 1.0 / 60.0; // 60 FPS
    
    for (BaseBallData* ball : m_allBalls) {
        if (ball && !ball->isRemoved()) {
            ball->updatePhysics(deltaTime);
        }
    }
    
    // 检查碰撞
    checkCollisions();
    
    // 检查玩家球合并
    QSet<QPair<int, int>> checkedPlayers;
    for (CloneBallData* player : m_players) {
        if (player && !player->isRemoved()) {
            QPair<int, int> playerKey(player->teamId(), player->playerId());
            if (!checkedPlayers.contains(playerKey)) {
                checkPlayerBallsMerging(player->teamId(), player->playerId());
                checkedPlayers.insert(playerKey);
            }
        }
    }
    
    // 生成新的食物和荆棘
    spawnFood();
    spawnThorns();
    
    // 清理已移除的球
    QVector<BaseBallData*> ballsToRemove;
    for (BaseBallData* ball : m_allBalls) {
        if (ball && ball->isRemoved()) {
            ballsToRemove.append(ball);
        }
    }
    
    for (BaseBallData* ball : ballsToRemove) {
        removeBall(ball);
        delete ball; // 直接删除，不使用deleteLater
    }
}

void GameEngine::spawnFood()
{
    m_foodRefreshFrameCount++;
    
    if (m_foodRefreshFrameCount >= m_config.foodRefreshFrames) {
        int currentFoodCount = m_foodBalls.size();
        int leftNum = m_config.maxFoodCount - currentFoodCount;
        
        if (leftNum > 0) {
            int todoNum = std::min(
                static_cast<int>(std::ceil(m_config.foodRefreshPercent * leftNum)), 
                leftNum
            );
            
            for (int i = 0; i < todoNum; ++i) {
                QPointF pos = generateRandomFoodPosition();
                FoodBallData* food = new FoodBallData(getNextBallId(), pos,
                    BaseBallData::Border(m_config.gameBorder.minx, m_config.gameBorder.maxx,
                                       m_config.gameBorder.miny, m_config.gameBorder.maxy));
                addBall(food);
            }
        }
        
        m_foodRefreshFrameCount = 0;
    }
}

void GameEngine::spawnThorns()
{
    m_thornsRefreshFrameCount++;
    
    if (m_thornsRefreshFrameCount >= m_config.thornsRefreshFrames) {
        int currentThornsCount = m_thornsBalls.size();
        int leftNum = m_config.maxThornsCount - currentThornsCount;
        
        if (leftNum > 0) {
            int todoNum = std::min(
                static_cast<int>(std::ceil(m_config.thornsRefreshPercent * leftNum)), 
                leftNum
            );
            
            for (int i = 0; i < todoNum; ++i) {
                QPointF pos = generateRandomThornsPosition();
                int score = m_config.thornsScoreMin + 
                           QRandomGenerator::global()->bounded(m_config.thornsScoreMax - m_config.thornsScoreMin + 1);
                ThornsBallData* thorns = new ThornsBallData(getNextBallId(), pos,
                    BaseBallData::Border(m_config.gameBorder.minx, m_config.gameBorder.maxx,
                                       m_config.gameBorder.miny, m_config.gameBorder.maxy));
                thorns->setScore(score);
                addBall(thorns);
            }
        }
        
        m_thornsRefreshFrameCount = 0;
    }
}

void GameEngine::checkCollisions()
{
    // 重建四叉树
    QVector<BaseBallData*> allBalls = getAllBalls();
    m_quadTree->rebuild(allBalls);
    
    // 获取移动的球体
    QVector<BaseBallData*> movingBalls;
    
    // 玩家球总是移动的
    for (CloneBallData* player : m_players) {
        if (player && !player->isRemoved()) {
            movingBalls.append(player);
        }
    }
    
    // 孢子球总是移动的
    for (SporeBallData* spore : m_sporeBalls) {
        if (spore && !spore->isRemoved()) {
            movingBalls.append(spore);
        }
    }
    
    // 移动中的荆棘球
    for (ThornsBallData* thorns : m_thornsBalls) {
        if (thorns && !thorns->isRemoved() && thorns->isMoving()) {
            movingBalls.append(thorns);
        }
    }
    
    // 检查移动球体的碰撞
    for (BaseBallData* movingBall : movingBalls) {
        if (!movingBall || movingBall->isRemoved()) continue;
        
        QVector<BaseBallData*> candidates = m_quadTree->queryCollisions(movingBall);
        
        for (BaseBallData* candidate : candidates) {
            if (!candidate || candidate->isRemoved() || candidate == movingBall) continue;
            
            if (movingBall->collidesWith(candidate)) {
                checkCollisionsBetween(movingBall, candidate);
            }
        }
    }
}

void GameEngine::checkCollisionsBetween(BaseBallData* ball1, BaseBallData* ball2)
{
    // 玩家球 vs 食物球
    if ((ball1->ballType() == BaseBallData::CLONE_BALL && ball2->ballType() == BaseBallData::FOOD_BALL) ||
        (ball1->ballType() == BaseBallData::FOOD_BALL && ball2->ballType() == BaseBallData::CLONE_BALL)) {
        
        CloneBallData* player = (ball1->ballType() == BaseBallData::CLONE_BALL) ? 
                               static_cast<CloneBallData*>(ball1) : static_cast<CloneBallData*>(ball2);
        FoodBallData* food = (ball1->ballType() == BaseBallData::FOOD_BALL) ? 
                            static_cast<FoodBallData*>(ball1) : static_cast<FoodBallData*>(ball2);
        
        if (player->canEat(food)) {
            player->eat(food);
        }
    }
    
    // 玩家球 vs 孢子球
    else if ((ball1->ballType() == BaseBallData::CLONE_BALL && ball2->ballType() == BaseBallData::SPORE_BALL) ||
             (ball1->ballType() == BaseBallData::SPORE_BALL && ball2->ballType() == BaseBallData::CLONE_BALL)) {
        
        CloneBallData* player = (ball1->ballType() == BaseBallData::CLONE_BALL) ? 
                               static_cast<CloneBallData*>(ball1) : static_cast<CloneBallData*>(ball2);
        SporeBallData* spore = (ball1->ballType() == BaseBallData::SPORE_BALL) ? 
                              static_cast<SporeBallData*>(ball1) : static_cast<SporeBallData*>(ball2);
        
        if (spore->canBeEaten() && player->canEat(spore)) {
            player->eat(spore);
        }
    }
    
    // 玩家球 vs 玩家球
    else if (ball1->ballType() == BaseBallData::CLONE_BALL && ball2->ballType() == BaseBallData::CLONE_BALL) {
        CloneBallData* player1 = static_cast<CloneBallData*>(ball1);
        CloneBallData* player2 = static_cast<CloneBallData*>(ball2);
        
        // 同队玩家球的处理
        if (player1->teamId() == player2->teamId() && player1->playerId() == player2->playerId()) {
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
    
    // 荆棘球 vs 孢子球
    else if ((ball1->ballType() == BaseBallData::THORNS_BALL && ball2->ballType() == BaseBallData::SPORE_BALL) ||
             (ball1->ballType() == BaseBallData::SPORE_BALL && ball2->ballType() == BaseBallData::THORNS_BALL)) {
        
        ThornsBallData* thorns = (ball1->ballType() == BaseBallData::THORNS_BALL) ? 
                                static_cast<ThornsBallData*>(ball1) : static_cast<ThornsBallData*>(ball2);
        SporeBallData* spore = (ball1->ballType() == BaseBallData::SPORE_BALL) ? 
                              static_cast<SporeBallData*>(ball1) : static_cast<SporeBallData*>(ball2);
        
        thorns->eatSpore(spore);
    }
    
    // 玩家球 vs 荆棘球
    else if ((ball1->ballType() == BaseBallData::CLONE_BALL && ball2->ballType() == BaseBallData::THORNS_BALL) ||
             (ball1->ballType() == BaseBallData::THORNS_BALL && ball2->ballType() == BaseBallData::CLONE_BALL)) {
        
        CloneBallData* player = (ball1->ballType() == BaseBallData::CLONE_BALL) ? 
                               static_cast<CloneBallData*>(ball1) : static_cast<CloneBallData*>(ball2);
        ThornsBallData* thorns = (ball1->ballType() == BaseBallData::THORNS_BALL) ? 
                                static_cast<ThornsBallData*>(ball1) : static_cast<ThornsBallData*>(ball2);
        
        if (player->canEat(thorns)) {
            // 吃荆棘球触发特殊分裂
            int totalPlayerBalls = getPlayerBalls(player->teamId(), player->playerId()).size();
            QVector<CloneBallData*> newBalls = player->performThornsSplit(QVector2D(1, 0), totalPlayerBalls);
            
            for (CloneBallData* newBall : newBalls) {
                newBall->setBallId(getNextBallId());
                addBall(newBall);
                m_players.append(newBall);
            }
            
            // 移除荆棘球
            thorns->markAsRemoved();
        }
    }
}

void GameEngine::checkPlayerBallsMerging(int teamId, int playerId)
{
    QVector<CloneBallData*> playerBalls = getPlayerBalls(teamId, playerId);
    
    for (int i = 0; i < playerBalls.size(); ++i) {
        CloneBallData* ball1 = playerBalls[i];
        if (!ball1 || ball1->isRemoved()) continue;
        
        for (int j = i + 1; j < playerBalls.size(); ++j) {
            CloneBallData* ball2 = playerBalls[j];
            if (!ball2 || ball2->isRemoved()) continue;
            
            if (ball1->canMergeWith(ball2)) {
                ball1->mergeWith(ball2);
                return; // 一次只合并一对球
            }
        }
    }
}

QVector<CloneBallData*> GameEngine::getPlayerBalls(int teamId, int playerId) const
{
    QVector<CloneBallData*> playerBalls;
    
    for (CloneBallData* player : m_players) {
        if (player && !player->isRemoved() && 
            player->teamId() == teamId && player->playerId() == playerId) {
            playerBalls.append(player);
        }
    }
    
    return playerBalls;
}

Observation GameEngine::getObservation() const
{
    Observation obs;
    
    // 全局状态
    obs.global_state.border = {static_cast<int>(m_config.gameBorder.maxx - m_config.gameBorder.minx),
                              static_cast<int>(m_config.gameBorder.maxy - m_config.gameBorder.miny)};
    obs.global_state.total_frame = m_totalFrames;
    obs.global_state.last_frame_count = 1;
    
    // 计算排行榜
    QMap<int, float> teamScores;
    for (CloneBallData* player : m_players) {
        if (player && !player->isRemoved()) {
            teamScores[player->teamId()] += player->score();
        }
    }
    obs.global_state.leaderboard = teamScores;
    
    // 玩家状态
    QSet<int> processedPlayers;
    for (CloneBallData* player : m_players) {
        if (player && !player->isRemoved()) {
            int playerId = player->playerId();
            if (!processedPlayers.contains(playerId)) {
                obs.player_states[playerId] = getPlayerState(playerId);
                processedPlayers.insert(playerId);
            }
        }
    }
    
    return obs;
}

PlayerState GameEngine::getPlayerState(int playerId) const
{
    PlayerState state;
    
    // 计算玩家视野矩形
    QRectF viewRect = calculatePlayerViewRect(playerId);
    state.rectangle = {static_cast<float>(viewRect.left()), static_cast<float>(viewRect.top()),
                      static_cast<float>(viewRect.right()), static_cast<float>(viewRect.bottom())};
    
    // 获取视野内的对象
    QVector<QVector<float>> rawFood = getObjectsInView(viewRect, BaseBallData::FOOD_BALL);
    QVector<QVector<float>> rawThorns = getObjectsInView(viewRect, BaseBallData::THORNS_BALL);
    QVector<QVector<float>> rawSpore = getObjectsInView(viewRect, BaseBallData::SPORE_BALL);
    QVector<QVector<float>> rawClone = getObjectsInView(viewRect, BaseBallData::CLONE_BALL);
    
    // 计算玩家中心位置用于距离排序
    QPointF playerCenter = viewRect.center();
    
    // 按距离排序后再预处理
    state.food = preprocessObjects(sortObjectsByDistance(rawFood, playerCenter), 50);
    state.thorns = preprocessObjects(sortObjectsByDistance(rawThorns, playerCenter), 20);
    state.spore = preprocessObjects(sortObjectsByDistance(rawSpore, playerCenter), 10);
    state.clone = preprocessObjects(sortObjectsByDistance(rawClone, playerCenter), 30);
    
    // 计算玩家总分数和能力
    float totalScore = getTotalPlayerScore(0, playerId); // 假设team 0
    state.score = totalScore;
    
    // 检查主玩家的能力（取第一个未移除的球）
    for (CloneBallData* player : m_players) {
        if (player && !player->isRemoved() && player->playerId() == playerId) {
            state.can_eject = player->canEject();
            state.can_split = player->canSplit();
            break;
        }
    }
    
    return state;
}

QVector<QVector<float>> GameEngine::getObjectsInView(const QRectF& viewRect, BaseBallData::BallType type) const
{
    QVector<QVector<float>> objects;
    
    // 正确遍历QMap
    for (auto it = m_allBalls.constBegin(); it != m_allBalls.constEnd(); ++it) {
        BaseBallData* ball = it.value();
        if (!ball || ball->isRemoved() || ball->ballType() != type) continue;
        
        QPointF pos = ball->position();
        if (!viewRect.contains(pos)) continue;
        
        QVector<float> objData;
        
        // 获取地图尺寸用于归一化
        float mapWidth = m_config.gameBorder.maxx - m_config.gameBorder.minx;
        float mapHeight = m_config.gameBorder.maxy - m_config.gameBorder.miny;
        
        // 归一化位置 (相对于地图中心，范围[-1,1])
        float x = (pos.x() - (m_config.gameBorder.minx + m_config.gameBorder.maxx) / 2.0f) / (mapWidth / 2.0f);
        float y = (pos.y() - (m_config.gameBorder.miny + m_config.gameBorder.maxy) / 2.0f) / (mapHeight / 2.0f);
        
        // 归一化半径和分数
        float radius = ball->radius() / 100.0f; // 假设最大半径为100
        float score = ball->score() / 1000.0f;  // 假设分数范围为0-1000
        
        switch (type) {
            case BaseBallData::FOOD_BALL:
                // 食物: [x, y, radius, score]
                objData = {x, y, radius, score};
                break;
                
            case BaseBallData::THORNS_BALL:
                // 荆棘: [x, y, radius, score, vx, vy]
                {
                    QVector2D vel = ball->velocity();
                    objData = {x, y, radius, score, vel.x() / 100.0f, vel.y() / 100.0f};
                }
                break;
                
            case BaseBallData::SPORE_BALL:
                // 孢子: [x, y, radius, score, vx, vy]
                {
                    QVector2D vel = ball->velocity();
                    objData = {x, y, radius, score, vel.x() / 100.0f, vel.y() / 100.0f};
                }
                break;
                
            case BaseBallData::CLONE_BALL:
                // 克隆球: [x, y, radius, score, vx, vy, dir_x, dir_y, team_id, player_id]
                {
                    CloneBallData* clone = static_cast<CloneBallData*>(ball);
                    QVector2D vel = ball->velocity();
                    QVector2D dir = clone->moveDirection();
                    objData = {x, y, radius, score, vel.x() / 100.0f, vel.y() / 100.0f,
                              dir.x(), dir.y(), static_cast<float>(clone->teamId()), static_cast<float>(clone->playerId())};
                }
                break;
        }
        
        if (!objData.isEmpty()) {
            objects.append(objData);
        }
    }
    
    return objects;
}

QRectF GameEngine::calculatePlayerViewRect(int playerId) const
{
    // 找到所有属于该玩家的球
    QVector<CloneBallData*> playerBalls = getPlayerBalls(0, playerId); // 假设team 0
    
    if (playerBalls.isEmpty()) {
        return QRectF(0, 0, 1000, 1000); // 默认视野
    }
    
    // 计算质心和最大半径
    QPointF centroid(0, 0);
    float totalScore = 0;
    float maxRadius = 0;
    
    for (CloneBallData* ball : playerBalls) {
        if (ball && !ball->isRemoved()) {
            float score = ball->score();
            centroid += ball->position() * score;
            totalScore += score;
            maxRadius = qMax(maxRadius, ball->radius());
        }
    }
    
    if (totalScore > 0) {
        centroid /= totalScore;
    }
    
    // 计算视野大小（基于GoBigger算法）
    float visionSize = maxRadius * 8.0f; // 8倍最大球半径
    visionSize = qMax(visionSize, 400.0f); // 最小视野
    visionSize = qMin(visionSize, 1200.0f); // 最大视野
    
    return QRectF(centroid.x() - visionSize, centroid.y() - visionSize,
                  visionSize * 2, visionSize * 2);
}

QVector<QVector<float>> GameEngine::preprocessObjects(const QVector<QVector<float>>& objects, int maxCount) const
{
    QVector<QVector<float>> result = objects;
    
    // 如果超出数量限制，按距离排序并裁剪（距离玩家中心）
    if (result.size() > maxCount) {
        // TODO: 实现更精确的距离排序
        // 目前简单截取前maxCount个
        result = result.mid(0, maxCount);
    }
    
    // 确定特征向量的维度
    int featureCount = 4; // 默认维度
    if (!objects.isEmpty()) {
        featureCount = objects.first().size();
    }
    
    // 如果不足数量，用0填充
    while (result.size() < maxCount) {
        QVector<float> zeros(featureCount, 0.0f);
        result.append(zeros);
    }
    
    return result;
}

QVector<QVector<float>> GameEngine::sortObjectsByDistance(const QVector<QVector<float>>& objects, const QPointF& playerCenter) const
{
    QVector<QPair<float, QVector<float>>> objectsWithDistance;
    
    // 计算每个对象到玩家中心的距离
    for (const QVector<float>& obj : objects) {
        if (obj.size() >= 2) {
            // 假设前两个元素是x,y坐标（已归一化）
            float dx = obj[0] * (m_config.gameBorder.maxx - m_config.gameBorder.minx) / 2.0f - playerCenter.x();
            float dy = obj[1] * (m_config.gameBorder.maxy - m_config.gameBorder.miny) / 2.0f - playerCenter.y();
            float distance = sqrt(dx * dx + dy * dy);
            objectsWithDistance.append({distance, obj});
        }
    }
    
    // 按距离排序（从近到远）
    std::sort(objectsWithDistance.begin(), objectsWithDistance.end(),
              [](const QPair<float, QVector<float>>& a, const QPair<float, QVector<float>>& b) {
                  return a.first < b.first;
              });
    
    // 提取排序后的对象
    QVector<QVector<float>> sortedObjects;
    for (const auto& pair : objectsWithDistance) {
        sortedObjects.append(pair.second);
    }
    
    return sortedObjects;
}
