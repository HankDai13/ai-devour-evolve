// 先包含pybind11避免宏冲突
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "multi_agent_game_engine.h"
#include "../src_new/GoBiggerConfig.h"
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QDebug>
#include <cmath>

MultiAgentGameEngine::MultiAgentGameEngine(const Config& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_scene(nullptr)
    , m_gameManager(nullptr)
    , m_multiPlayerManager(nullptr)
    , m_frameCount(0)
    , m_gameRunning(false)
    , m_rlPlayerTeamId(-1)
    , m_rlPlayerPlayerId(-1)
{
    // 确保QApplication存在
    if (!QApplication::instance()) {
        static int argc = 1;
        static char* argv[] = {const_cast<char*>("multi_agent_env"), nullptr};
        static QApplication app(argc, argv);
    }
    
    // 创建游戏场景
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(
        -GoBiggerConfig::MAP_WIDTH/2, -GoBiggerConfig::MAP_HEIGHT/2,
        GoBiggerConfig::MAP_WIDTH, GoBiggerConfig::MAP_HEIGHT
    );
    
    // 创建游戏管理器
    GameManager::Config gameConfig;
    gameConfig.maxFoodCount = m_config.maxFoodCount;
    gameConfig.initFoodCount = m_config.initFoodCount;
    gameConfig.maxThornsCount = m_config.maxThornsCount;
    gameConfig.initThornsCount = m_config.initThornsCount;
    gameConfig.gameUpdateInterval = m_config.gameUpdateInterval;
    
    m_gameManager = new GameManager(m_scene, gameConfig, this);
    
    // 创建多玩家管理器
    m_multiPlayerManager = new GoBigger::Multiplayer::MultiPlayerManager(m_gameManager, this);
    
    qDebug() << "MultiAgentGameEngine initialized";
}

MultiAgentGameEngine::~MultiAgentGameEngine()
{
    if (m_multiPlayerManager) {
        m_multiPlayerManager->stopMultiPlayerGame();
    }
}

py::dict MultiAgentGameEngine::reset()
{
    qDebug() << "Resetting multi-agent environment";
    
    // 停止当前游戏
    if (m_gameRunning) {
        m_multiPlayerManager->stopMultiPlayerGame();
    }
    
    // 清理所有玩家
    m_multiPlayerManager->removeAllPlayers();
    
    // 重置游戏状态
    m_gameManager->resetGame();
    m_frameCount = 0;
    m_gameRunning = false;
    
    // 设置RL智能体（团队0，玩家0）
    m_rlPlayerTeamId = 0;
    m_rlPlayerPlayerId = 0;
    
    // 添加RL智能体玩家
    GoBigger::Multiplayer::PlayerInfo rlPlayer(
        m_rlPlayerTeamId, m_rlPlayerPlayerId, 
        GoBigger::Multiplayer::PlayerType::HUMAN, 
        "RL_Agent", ""
    );
    
    if (!m_multiPlayerManager->addPlayer(rlPlayer)) {
        qWarning() << "Failed to add RL player";
        return py::dict();
    }
    
    // 创建RL玩家的物理球体
    CloneBall* rlBall = m_gameManager->createPlayer(
        m_rlPlayerTeamId, m_rlPlayerPlayerId, 
        QPointF(0, 0)  // 出生在地图中心
    );
    
    if (!rlBall) {
        qWarning() << "Failed to create RL player ball";
        return py::dict();
    }
    
    // 添加传统AI对手
    setupTraditionalAIOpponents();
    
    // 启动游戏
    m_multiPlayerManager->startMultiPlayerGame();
    m_gameRunning = true;
    
    qDebug() << "Multi-agent environment reset complete";
    return getObservation();
}

py::dict MultiAgentGameEngine::step(const py::dict& actions)
{
    if (!m_gameRunning) {
        qWarning() << "Game not running, cannot step";
        return py::dict();
    }
    
    // 执行RL智能体的动作
    if (actions.contains("rl_agent")) {
        py::object rlActionObj = actions["rl_agent"];
        py::list rlAction = rlActionObj.cast<py::list>();
        if (rlAction.size() >= 3) {
            executeRLAction(rlAction);
        }
    }
    
    // 手动更新游戏状态（这是关键！）
    m_gameManager->manualUpdateGame();    // 更新物理状态、碰撞检测
    m_gameManager->manualSpawnFood();     // 补充食物
    m_gameManager->manualSpawnThorns();   // 补充荆棘球
    
    // 让游戏管理器更新一帧
    // 注意：传统AI会自动执行他们的决策
    m_frameCount++;
    
    // 处理Qt事件循环以更新游戏状态
    QApplication::processEvents();
    
    return getObservation();
}

bool MultiAgentGameEngine::isDone() const
{
    if (!m_gameRunning) {
        return true;
    }
    
    // 检查RL智能体是否还存活
    CloneBall* rlBall = m_gameManager->getPlayer(m_rlPlayerTeamId, m_rlPlayerPlayerId);
    if (!rlBall || !rlBall->isActive()) {
        return true;
    }
    
    // 检查是否达到最大帧数
    if (m_frameCount >= m_config.maxFrames) {
        return true;
    }
    
    // 检查是否还有其他活跃玩家
    auto players = m_gameManager->getPlayers();
    int activePlayerCount = 0;
    for (auto player : players) {
        if (player && player->isActive()) {
            activePlayerCount++;
        }
    }
    
    return activePlayerCount <= 1;  // 只剩一个或零个玩家时游戏结束
}

py::dict MultiAgentGameEngine::getObservation() const
{
    py::dict observation;
    
    // 全局状态
    py::dict globalState;
    globalState["frame"] = m_frameCount;
    globalState["total_players"] = m_gameManager->getPlayerCount();
    globalState["food_count"] = m_gameManager->getFoodCount();
    globalState["thorns_count"] = m_gameManager->getThornsCount();
    
    // 计算团队排名
    auto teamRanking = calculateTeamRanking();
    globalState["team_ranking"] = teamRanking;
    
    observation["global_state"] = globalState;
    
    // RL智能体的观察
    CloneBall* rlBall = m_gameManager->getPlayer(m_rlPlayerTeamId, m_rlPlayerPlayerId);
    if (rlBall) {
        observation["rl_agent"] = extractPlayerObservation(rlBall);
    } else {
        observation["rl_agent"] = py::dict();  // 空观察表示智能体已死亡
    }
    
    // 传统AI的状态（用于分析和调试）
    py::dict aiStates;
    auto aiPlayers = m_gameManager->getAIPlayers();
    for (int i = 0; i < aiPlayers.size(); ++i) {
        auto aiPlayer = aiPlayers[i];
        if (aiPlayer && aiPlayer->hasAliveBalls()) {
            py::dict aiState;
            aiState["strategy"] = static_cast<int>(aiPlayer->getAIStrategy());
            aiState["active"] = aiPlayer->isAIActive();
            aiState["alive_balls_count"] = aiPlayer->getAllAliveBalls().size();
            
            CloneBall* largestBall = aiPlayer->getLargestBall();
            if (largestBall) {
                aiState["score"] = largestBall->score();
                aiState["position"] = py::make_tuple(largestBall->x(), largestBall->y());
            }
            
            std::string aiKey = QString("ai_%1").arg(i).toStdString();
            aiStates[py::str(aiKey)] = aiState;
        }
    }
    observation["ai_states"] = aiStates;
    
    return observation;
}

py::dict MultiAgentGameEngine::getRewardInfo() const
{
    py::dict rewardInfo;
    
    // RL智能体信息
    CloneBall* rlBall = m_gameManager->getPlayer(m_rlPlayerTeamId, m_rlPlayerPlayerId);
    if (rlBall) {
        rewardInfo["score"] = rlBall->score();
        rewardInfo["rl_score"] = rlBall->score();  // 添加RL分数别名
        rewardInfo["alive"] = rlBall->isActive();
        rewardInfo["can_split"] = rlBall->canSplit();
        rewardInfo["can_eject"] = rlBall->canEject();
    } else {
        rewardInfo["score"] = 0.0;
        rewardInfo["rl_score"] = 0.0;
        rewardInfo["alive"] = false;
        rewardInfo["can_split"] = false;
        rewardInfo["can_eject"] = false;
    }
    
    // AI对手分数信息
    for (int i = 0; i < m_config.aiOpponentCount; ++i) {
        int teamId = i + 1;  // AI团队从1开始
        int playerId = 0;
        
        CloneBall* aiBall = m_gameManager->getPlayer(teamId, playerId);
        if (aiBall) {
            QString aiScoreKey = QString("ai_%1_score").arg(i);
            rewardInfo[aiScoreKey.toStdString().c_str()] = aiBall->score();
        } else {
            QString aiScoreKey = QString("ai_%1_score").arg(i);
            rewardInfo[aiScoreKey.toStdString().c_str()] = 0.0;
        }
    }
    
    // 团队排名信息
    auto teamRanking = calculateTeamRanking();
    py::list rankingList;
    int rlTeamRank = static_cast<int>(teamRanking.size());  // 默认最后一名
    
    for (int i = 0; i < static_cast<int>(teamRanking.size()); ++i) {
        py::dict teamInfo = teamRanking[i].cast<py::dict>();
        rankingList.append(teamInfo);
        
        if (teamInfo["team_id"].cast<int>() == m_rlPlayerTeamId) {
            rlTeamRank = i + 1;  // 1-based ranking
        }
    }
    
    rewardInfo["team_rank"] = rlTeamRank;
    rewardInfo["total_teams"] = teamRanking.size();
    rewardInfo["team_ranking"] = rankingList;  // 添加完整排名信息
    
    return rewardInfo;
}

void MultiAgentGameEngine::setupTraditionalAIOpponents()
{
    // 设置不同策略的AI对手
    QVector<GoBigger::AI::AIStrategy> strategies = {
        GoBigger::AI::AIStrategy::FOOD_HUNTER,
        GoBigger::AI::AIStrategy::AGGRESSIVE,
        GoBigger::AI::AIStrategy::RANDOM
    };
    
    // 直接通过GameManager创建AI对手，不通过MultiPlayerManager
    for (int i = 0; i < m_config.aiOpponentCount; ++i) {
        int teamId = i + 1;  // 从团队1开始（团队0是RL智能体）
        int playerId = 0;
        
        // 选择策略
        GoBigger::AI::AIStrategy strategy = strategies[i % strategies.size()];
        
        // 直接通过GameManager添加AI玩家
        if (m_gameManager->addAIPlayerWithStrategy(teamId, playerId, strategy)) {
            qDebug() << "Added AI opponent" << i << "with strategy" << static_cast<int>(strategy)
                     << "on team" << teamId;
        } else {
            qWarning() << "Failed to add AI opponent" << i << "on team" << teamId;
        }
    }
}

void MultiAgentGameEngine::executeRLAction(const py::list& action)
{
    if (action.size() < 3) {
        qWarning() << "Invalid RL action size";
        return;
    }
    
    CloneBall* rlBall = m_gameManager->getPlayer(m_rlPlayerTeamId, m_rlPlayerPlayerId);
    if (!rlBall || !rlBall->isActive()) {
        return;
    }
    
    // 解析动作
    float dx = action[0].cast<float>();
    float dy = action[1].cast<float>();
    int actionType = action[2].cast<int>();
    
    // 裁剪到有效范围
    dx = std::max(-1.0f, std::min(1.0f, dx));
    dy = std::max(-1.0f, std::min(1.0f, dy));
    actionType = std::max(0, std::min(2, actionType));
    
    // 执行移动
    if (dx != 0.0f || dy != 0.0f) {
        QPointF direction(dx, dy);
        rlBall->setTargetDirection(direction);
    }
    
    // 执行特殊动作
    switch (actionType) {
        case 1:  // Split
            if (rlBall->canSplit()) {
                QVector2D splitDirection(dx, dy);
                if (splitDirection.length() == 0) {
                    splitDirection = QVector2D(1, 0);  // 默认向右分裂
                }
                splitDirection.normalize();
                rlBall->performSplit(splitDirection);
            }
            break;
        case 2:  // Eject
            if (rlBall->canEject()) {
                QVector2D ejectDirection(dx, dy);
                if (ejectDirection.length() == 0) {
                    ejectDirection = QVector2D(1, 0);  // 默认向右喷射
                }
                ejectDirection.normalize();
                rlBall->ejectSpore(ejectDirection);
            }
            break;
        default:  // Move only
            break;
    }
}

py::dict MultiAgentGameEngine::extractPlayerObservation(CloneBall* player) const
{
    py::dict obs;
    
    if (!player) {
        return obs;
    }
    
    // 玩家基础信息
    obs["position"] = py::make_tuple(player->x(), player->y());
    obs["radius"] = player->radius();
    obs["score"] = player->score();
    obs["velocity"] = py::make_tuple(player->velocity().x(), player->velocity().y());
    obs["can_split"] = player->canSplit();
    obs["can_eject"] = player->canEject();
    
    // 视野范围内的对象（简化版本）
    py::list nearbyFood;
    py::list nearbyThorns;
    py::list nearbyPlayers;
    
    // 获取视野范围内的食物
    auto foodBalls = m_gameManager->getFoodBallsInRect(
        QRectF(player->x() - 200, player->y() - 200, 400, 400)
    );
    for (auto food : foodBalls) {
        if (food) {
            nearbyFood.append(py::make_tuple(
                food->x(), food->y(), food->radius(), food->score()
            ));
        }
    }
    
    // 获取视野范围内的其他玩家
    auto allPlayers = m_gameManager->getPlayers();
    for (auto otherPlayer : allPlayers) {
        if (otherPlayer && otherPlayer != player && otherPlayer->isActive()) {
            float distance = QVector2D(
                otherPlayer->x() - player->x(),
                otherPlayer->y() - player->y()
            ).length();
            
            if (distance <= 300.0f) {  // 视野范围
                nearbyPlayers.append(py::make_tuple(
                    otherPlayer->x(), otherPlayer->y(), 
                    otherPlayer->radius(), otherPlayer->score(),
                    otherPlayer->teamId(), otherPlayer->playerId()
                ));
            }
        }
    }
    
    obs["nearby_food"] = nearbyFood;
    obs["nearby_thorns"] = nearbyThorns;
    obs["nearby_players"] = nearbyPlayers;
    
    return obs;
}

py::list MultiAgentGameEngine::calculateTeamRanking() const
{
    QMap<int, float> teamScores;
    
    // 1. 计算普通玩家（包括RL智能体）的分数
    auto players = m_gameManager->getPlayers();
    for (auto player : players) {
        if (player) {
            int teamId = player->teamId();
            teamScores[teamId] += player->score();
            qDebug() << "Regular player - Team:" << teamId << "Score:" << player->score();
        }
    }
    
    // 2. 计算AI玩家的分数
    auto aiPlayers = m_gameManager->getAIPlayers();
    for (auto aiPlayer : aiPlayers) {
        if (aiPlayer && aiPlayer->getPlayerBall()) {
            CloneBall* aiBall = aiPlayer->getPlayerBall();
            int teamId = aiBall->teamId();
            teamScores[teamId] += aiBall->score();
            qDebug() << "AI player - Team:" << teamId << "Score:" << aiBall->score();
        }
    }
    
    // 如果没有找到任何玩家，初始化基础团队结构
    if (teamScores.isEmpty()) {
        qWarning() << "No players found, initializing default team structure";
        teamScores[m_rlPlayerTeamId] = 0.0f;
        for (int i = 0; i < m_config.aiOpponentCount; ++i) {
            teamScores[i + 1] = 0.0f;
        }
    }
    
    qDebug() << "Team scores calculated:" << teamScores;
    
    // 转换为排序列表
    QVector<QPair<int, float>> sortedTeams;
    for (auto it = teamScores.begin(); it != teamScores.end(); ++it) {
        sortedTeams.append(qMakePair(it.key(), it.value()));
    }
    
    // 按分数降序排序
    std::sort(sortedTeams.begin(), sortedTeams.end(),
              [](const QPair<int, float>& a, const QPair<int, float>& b) {
                  return a.second > b.second;
              });
    
    // 转换为Python列表
    py::list ranking;
    for (int i = 0; i < sortedTeams.size(); ++i) {
        py::dict teamInfo;
        teamInfo["team_id"] = sortedTeams[i].first;
        teamInfo["score"] = sortedTeams[i].second;
        teamInfo["rank"] = i + 1;
        ranking.append(teamInfo);
    }
    
    qDebug() << "Final ranking size:" << ranking.size();
    return ranking;
}

QPointF MultiAgentGameEngine::generateRandomSpawnPosition() const
{
    // 在地图边缘生成随机位置，避免与RL智能体重叠
    float mapSize = GoBiggerConfig::MAP_WIDTH * 0.4f;  // 使用40%的地图大小
    float angle = QRandomGenerator::global()->generateDouble() * 2.0 * M_PI;
    float minDistance = mapSize * 0.5f;
    float maxDistance = mapSize;
    float distance = minDistance + QRandomGenerator::global()->generateDouble() * (maxDistance - minDistance);
    
    return QPointF(
        distance * std::cos(angle),
        distance * std::sin(angle)
    );
}
