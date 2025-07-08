#include "SimpleAIPlayer.h"
#include "ONNXInference.h"
#include "CloneBall.h"
#include "FoodBall.h"
#include "BaseBall.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QRandomGenerator>
#include <QPointF>
#include <QStandardPaths>
#include <QDir>
#include <cmath>
#include <algorithm>
#include <limits>

namespace GoBigger {
namespace AI {

// SimpleAIPlayer 实现
SimpleAIPlayer::SimpleAIPlayer(CloneBall* playerBall, QObject* parent)
    : QObject(parent)
    , m_playerBall(playerBall)
    , m_decisionTimer(new QTimer(this))
    , m_aiActive(false)
    , m_decisionInterval(200) // 默认200ms决策间隔
    , m_strategy(AIStrategy::FOOD_HUNTER) // 默认食物猎手策略
    , m_currentTarget(nullptr)
    , m_targetLockFrames(0)
    , m_onnxInference(std::make_unique<ONNXInference>())
    , m_observationSize(400) // 默认观察向量大小
    , m_stuckFrameCount(0)
    , m_lastPosition(0, 0)
    , m_borderCollisionCount(0)
{
    // 连接定时器
    connect(m_decisionTimer, &QTimer::timeout, this, &SimpleAIPlayer::makeDecision);
    
    // 监听玩家球被销毁的信号
    if (m_playerBall) {
        connect(m_playerBall, &QObject::destroyed, this, &SimpleAIPlayer::onPlayerBallDestroyed);
        // 监听球被移除的信号
        connect(m_playerBall, &BaseBall::ballRemoved, this, &SimpleAIPlayer::onPlayerBallRemoved);
        // 监听分裂信号
        connect(m_playerBall, &CloneBall::splitPerformed, this, &SimpleAIPlayer::onSplitPerformed);
        
        // 初始化分裂球列表
        m_splitBalls.clear();
        m_splitBalls.append(m_playerBall);
        
        // 初始化位置记录
        m_lastPosition = m_playerBall->pos();
    }
    
    qDebug() << "SimpleAIPlayer created for ball:" << (m_playerBall ? m_playerBall->ballId() : -1)
             << "with strategy:" << static_cast<int>(m_strategy);
}

SimpleAIPlayer::~SimpleAIPlayer() {
    stopAI();
    qDebug() << "SimpleAIPlayer destroyed";
}

void SimpleAIPlayer::startAI() {
    if (!m_playerBall) {
        qWarning() << "Cannot start AI: no player ball";
        return;
    }
    
    if (m_aiActive) {
        qDebug() << "AI already active";
        return;
    }
    
    m_aiActive = true;
    m_decisionTimer->start(m_decisionInterval);
    qDebug() << "AI started for player ball:" << m_playerBall->ballId() 
             << "with decision interval:" << m_decisionInterval << "ms"
             << "strategy:" << static_cast<int>(m_strategy);
}

void SimpleAIPlayer::stopAI() {
    if (!m_aiActive) {
        return;
    }
    
    m_aiActive = false;
    m_decisionTimer->stop();
    qDebug() << "AI stopped for player ball:" << (m_playerBall ? m_playerBall->ballId() : -1);
}

void SimpleAIPlayer::setDecisionInterval(int interval_ms) {
    m_decisionInterval = std::max(50, interval_ms); // 最小50ms
    if (m_aiActive) {
        m_decisionTimer->start(m_decisionInterval);
    }
}

void SimpleAIPlayer::makeDecision() {
    if (!m_aiActive || m_splitBalls.isEmpty()) {
        return;
    }
    
    try {
        // 🔥 修复：为每个分裂球独立决策，而不是统一行动
        for (CloneBall* ball : m_splitBalls) {
            if (!ball || ball->isRemoved()) continue;
            
            // 临时设置当前控制的球，用于各种检测和决策
            CloneBall* originalPlayerBall = m_playerBall;
            m_playerBall = ball;
            
            AIAction action;
            
            // 🔥 分裂球协调逻辑：只有在严重分散时才强制聚拢
            if (m_splitBalls.size() > 1) {
                // 计算球群质心和分散度
                QPointF centroid(0, 0);
                float totalScore = 0.0f;
                int validBalls = 0;
                
                for (CloneBall* otherBall : m_splitBalls) {
                    if (otherBall && !otherBall->isRemoved()) {
                        QPointF pos = otherBall->pos();
                        float score = otherBall->score();
                        centroid += pos * score;
                        totalScore += score;
                        validBalls++;
                    }
                }
                
                if (validBalls > 1 && totalScore > 0) {
                    centroid /= totalScore;
                    
                    QPointF ballPos = ball->pos();
                    float distanceToCenter = QLineF(ballPos, centroid).length();
                    
                    // 只有距离质心太远时才强制聚拢
                    const float criticalDistance = 200.0f; // 提高临界距离
                    if (distanceToCenter > criticalDistance) {
                        qDebug() << "Ball" << ball->ballId() << "too far from group (" 
                                 << distanceToCenter << "), forcing gather";
                        
                        QPointF direction = centroid - ballPos;
                        float length = QLineF(QPointF(0,0), direction).length();
                        if (length > 0) {
                            direction /= length;
                            action = AIAction(direction.x(), direction.y(), ActionType::MOVE);
                            executeActionForBall(ball, action);
                            m_playerBall = originalPlayerBall; // 恢复主球
                            continue; // 跳过正常决策
                        }
                    }
                }
            }
            
            // 🔥 为每个球独立执行策略决策
            switch (m_strategy) {
                case AIStrategy::RANDOM:
                    action = makeRandomDecision();
                    break;
                case AIStrategy::FOOD_HUNTER:
                    // 分裂状态下使用协调食物搜索，单球状态下使用普通食物搜索
                    if (m_splitBalls.size() > 1) {
                        action = makeCoordinatedFoodHunt();
                    } else {
                        action = makeFoodHunterDecision();
                    }
                    break;
                case AIStrategy::AGGRESSIVE:
                    action = makeAggressiveDecision();
                    break;
                case AIStrategy::MODEL_BASED:
                    action = makeModelBasedDecision();
                    break;
            }
            
            // 执行动作
            executeActionForBall(ball, action);
            
            // 恢复原始主球
            m_playerBall = originalPlayerBall;
        }
        
        // 发送第一个球的动作信号（用于UI显示）
        if (!m_splitBalls.isEmpty() && m_splitBalls.first()) {
            CloneBall* firstBall = m_splitBalls.first();
            CloneBall* originalPlayerBall = m_playerBall;
            m_playerBall = firstBall;
            AIAction displayAction = makeFoodHunterDecision();
            m_playerBall = originalPlayerBall;
            emit actionExecuted(displayAction);
        }
        
    } catch (const std::exception& e) {
        qWarning() << "AI decision error:" << e.what();
    }
}

void SimpleAIPlayer::onPlayerBallDestroyed() {
    qDebug() << "Player ball destroyed, stopping AI";
    m_playerBall = nullptr;
    stopAI();
}

void SimpleAIPlayer::onPlayerBallRemoved() {
    qDebug() << "Player ball removed/eaten, checking for other alive balls";
    
    // 🔥 优化：主球被移除时，不要立即停止AI，检查是否还有其他球存活
    if (m_playerBall) {
        // 从分裂球列表中移除被吃掉的球
        m_splitBalls.removeAll(m_playerBall);
        m_playerBall = nullptr;
    }
    
    // 如果还有其他球存活，切换到最大的球作为新的主控球
    if (!m_splitBalls.isEmpty()) {
        CloneBall* newMainBall = nullptr;
        float maxScore = 0.0f;
        
        // 找到最大的球
        for (CloneBall* ball : m_splitBalls) {
            if (ball && !ball->isRemoved() && ball->score() > maxScore) {
                maxScore = ball->score();
                newMainBall = ball;
            }
        }
        
        if (newMainBall) {
            m_playerBall = newMainBall;
            qDebug() << "Switched to new main ball:" << newMainBall->ballId() 
                     << "with score:" << newMainBall->score();
            return; // 继续AI控制
        }
    }
    
    // 如果没有球存活了，才停止AI
    qDebug() << "No alive balls remaining, stopping AI";
    stopAI();
    
    // 通知外部AI玩家已被销毁
    emit aiPlayerDestroyed(this);
}

AIAction SimpleAIPlayer::makeRandomDecision() {
    // 随机移动策略
    float dx = (QRandomGenerator::global()->generateDouble() - 0.5f) * 2.0f; // [-1, 1]
    float dy = (QRandomGenerator::global()->generateDouble() - 0.5f) * 2.0f; // [-1, 1]
    
    // 偶尔执行特殊动作
    ActionType actionType = ActionType::MOVE;
    int random = QRandomGenerator::global()->bounded(100);
    
    if (random < 5 && m_playerBall->canSplit()) { // 5%概率分裂
        actionType = ActionType::SPLIT;
    } else if (random < 10 && m_playerBall->canEject()) { // 5%概率喷射
        actionType = ActionType::EJECT;
    }
    
    return AIAction(dx, dy, actionType);
}

AIAction SimpleAIPlayer::makeFoodHunterDecision() {
    // Target locking logic
    if (m_currentTarget) {
        if (m_currentTarget->isRemoved() || !m_playerBall->canEat(m_currentTarget)) {
            m_currentTarget = nullptr;
            m_targetLockFrames = 0;
        } else {
            m_targetLockFrames++;
            if (m_targetLockFrames < 10) { // Lock target for 10 frames
                QPointF direction = m_currentTarget->pos() - m_playerBall->pos();
                float length = QLineF(QPointF(0,0), direction).length();
                if (length > 0.1f) {
                    QPointF safeDirection = getSafeDirection(direction / length);
                    return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
                }
            } else {
                m_currentTarget = nullptr; // Unlock after 10 frames
                m_targetLockFrames = 0;
            }
        }
    }

    QPointF playerPos = m_playerBall->pos();
    float playerRadius = m_playerBall->radius();
    float playerScore = m_playerBall->score();
    
    // 🔥 优化：增强威胁评估和食物密度分析
    auto nearbyPlayers = getNearbyPlayers(250.0f);
    auto nearbyBalls = getNearbyBalls(180.0f);
    auto nearbyFood = getNearbyFood(200.0f);
    
    // 1. 威胁评估 - 计算周围威胁等级
    QVector2D escapeDirection(0, 0);
    float totalThreatLevel = 0.0f;
    int highThreatCount = 0;
    
    for (auto player : nearbyPlayers) {
        if (player != m_playerBall && player->teamId() != m_playerBall->teamId()) {
            float distance = QLineF(player->pos(), playerPos).length();
            float threatScore = player->score();
            float radiusRatio = player->radius() / playerRadius;
            
            // 威胁级别：大小优势 × 距离因子
            if (threatScore > playerScore * 1.1f) {
                float sizeAdvantage = threatScore / playerScore;
                float distanceFactor = 1.0f / (distance / 100.0f + 1.0f);
                float threatLevel = sizeAdvantage * distanceFactor;
                
                totalThreatLevel += threatLevel;
                
                if (distance < 150.0f && sizeAdvantage > 1.3f) {
                    highThreatCount++;
                    QPointF awayDir = playerPos - player->pos();
                    float length = QLineF(QPointF(0,0), awayDir).length();
                    if (length > 0.1f) {
                        escapeDirection += QVector2D(awayDir / length) * threatLevel;
                    }
                }
            }
        }
    }        // 2. 紧急威胁处理 - 高威胁时分裂逃跑
        if (highThreatCount > 0 && totalThreatLevel > 3.0f) {
            escapeDirection = escapeDirection.normalized();
            qDebug() << "High threat detected, escaping! Threat level:" << totalThreatLevel;
            
            // 🔥 集成边界检测，确保逃跑方向安全
            QPointF safeEscapeDirection = getSafeDirection(QPointF(escapeDirection.x(), escapeDirection.y()));
            
            // 如果威胁极高且可以分裂，分裂逃跑
            if (totalThreatLevel > 5.0f && m_playerBall->canSplit() && playerScore > 30.0f) {
                return AIAction(safeEscapeDirection.x(), safeEscapeDirection.y(), ActionType::SPLIT);
            }
            
            return AIAction(safeEscapeDirection.x(), safeEscapeDirection.y(), ActionType::MOVE);
        }
    
    // 3. 荆棘球智能避障 - 优化避障逻辑防止打转
    for (auto ball : nearbyBalls) {
        if (ball->ballType() == BaseBall::THORNS_BALL) {
            float distance = QLineF(ball->pos(), playerPos).length();
            float thornsScore = ball->score();
            
            if (playerScore > thornsScore * 1.5f) {
                // 可以安全吃掉荆棘球
                if (distance < 80.0f && totalThreatLevel < 1.0f) {
                    QPointF direction = ball->pos() - playerPos;
                    float length = QLineF(QPointF(0,0), direction).length();
                    if (length > 0.1f) {
                        QPointF safeDirection = getSafeDirection(direction / length);
                        return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
                    }
                }
            } else if (distance < playerRadius + ball->radius() + 20.0f) {
                // 🔥 优化：动态安全距离，避免边缘打转
                float safeDistance = playerRadius + ball->radius() + 25.0f;
                QPointF awayDirection = playerPos - ball->pos();
                float awayLength = QLineF(QPointF(0,0), awayDirection).length();
                
                if (awayLength > 0.1f) {
                    // 计算垂直方向，避免直线逃离导致的打转
                    QPointF perpendicular(-awayDirection.y(), awayDirection.x());
                    QPointF finalDirection = awayDirection * 0.7f + perpendicular * 0.3f;
                    float finalLength = QLineF(QPointF(0,0), finalDirection).length();
                    
                    if (finalLength > 0.1f) {
                        finalDirection /= finalLength;
                        QPointF safeDirection = getSafeDirection(finalDirection);
                        return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
                    }
                }
            }
        }
    }
    
    // 4. 🔥 食物密度分析和智能分裂策略
    if (!nearbyFood.empty() && totalThreatLevel < 2.0f) {
        // 计算食物密度
        const float densityRadius = 80.0f;
        int foodDensity = 0;
        QPointF densityCenter(0, 0);
        
        for (auto food : nearbyFood) {
            QPointF foodPos = food->pos();
            float distanceToPlayer = QLineF(foodPos, playerPos).length();
            
            if (distanceToPlayer < densityRadius) {
                foodDensity++;
                densityCenter += foodPos;
            }
        }
        
        // 🔥 智能分裂：食物密度高且安全时分裂提高效率
        const int densityThreshold = 5; // 密度阈值
        if (foodDensity >= densityThreshold && m_playerBall->canSplit() && 
            playerScore > 25.0f && totalThreatLevel < 1.0f) {
            
            densityCenter /= foodDensity;
            QPointF direction = densityCenter - playerPos;
            float length = QLineF(QPointF(0,0), direction).length();
            
            if (length > 0.1f) {
                qDebug() << "High food density detected:" << foodDensity << "foods. Splitting for efficiency!";
                QPointF safeDirection = getSafeDirection(direction / length);
                return AIAction(safeDirection.x(), safeDirection.y(), ActionType::SPLIT);
            }
        }
        
        // 寻找最优食物目标（考虑价值、距离、安全性）
        FoodBall* bestFood = nullptr;
        float bestScore = -1.0f;
        
        for (auto food : nearbyFood) {
            QPointF foodPos = food->pos();
            float distance = QLineF(foodPos, playerPos).length();
            
            // 检查路径安全性
            bool pathSafe = true;
            for (auto player : nearbyPlayers) {
                if (player != m_playerBall && player->teamId() != m_playerBall->teamId() && 
                    player->score() > playerScore * 1.1f) {
                    float threatToFood = QLineF(player->pos(), foodPos).length();
                    if (threatToFood < 70.0f) {
                        pathSafe = false;
                        break;
                    }
                }
            }
            
            if (pathSafe) {
                // 评分：食物价值/距离 + 密度加成
                float localDensity = 0;
                for (auto otherFood : nearbyFood) {
                    if (QLineF(otherFood->pos(), foodPos).length() < 40.0f) {
                        localDensity += 1.0f;
                    }
                }
                
                float score = (food->score() / (distance + 1.0f)) * (1.0f + localDensity * 0.2f);
                if (score > bestScore) {
                    bestScore = score;
                    bestFood = food;
                }
            }
        }
        
        if (bestFood) {
            m_currentTarget = bestFood;
            QPointF direction = bestFood->pos() - playerPos;
            float length = QLineF(QPointF(0,0), direction).length();
            if (length > 0.1f) {
                QPointF safeDirection = getSafeDirection(direction / length);
                return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
            }
        }
    }
    
    // 5. 无明确目标时的探索策略
    if (totalThreatLevel < 1.0f) {
        // 向食物密度较高的区域移动
        QVector2D explorationDirection(0, 0);
        for (auto food : nearbyFood) {
            QPointF foodPos = food->pos();
            QPointF direction = foodPos - playerPos;
            float distance = QLineF(QPointF(0,0), direction).length();
            
            if (distance > 0.1f) {
                float weight = 1.0f / (distance / 100.0f + 1.0f);
                explorationDirection += QVector2D(direction / distance) * weight;
            }
        }
        
        if (explorationDirection.length() > 0.1f) {
            explorationDirection = explorationDirection.normalized();
            QPointF safeDirection = getSafeDirection(QPointF(explorationDirection.x(), explorationDirection.y()));
            return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
        }
    }
    
    // 6. 最后选择：随机探索
    return makeRandomDecision();
}

AIAction SimpleAIPlayer::makeCoordinatedDecision() {
    if (m_splitBalls.size() <= 1) {
        return makeFoodHunterDecision(); // Not in a split state
    }

    // 计算分裂球的质心作为聚集点
    QPointF centroid(0, 0);
    float totalScore = 0.0f;
    int validBalls = 0;
    
    for (CloneBall* ball : m_splitBalls) {
        if (ball && !ball->isRemoved()) {
            QPointF pos = ball->pos();
            float score = ball->score();
            centroid += pos * score; // 按分数加权
            totalScore += score;
            validBalls++;
        }
    }
    
    if (validBalls == 0) {
        return makeFoodHunterDecision();
    }
    
    centroid /= totalScore;

    // 检查分裂球是否分散过度
    float maxDistance = 0.0f;
    float avgDistance = 0.0f;
    for (CloneBall* ball : m_splitBalls) {
        if (ball && !ball->isRemoved()) {
            float distance = QLineF(ball->pos(), centroid).length();
            maxDistance = std::max(maxDistance, distance);
            avgDistance += distance;
        }
    }
    avgDistance /= validBalls;
    
    // 如果球分散太远，强制聚拢（即使在冷却期）
    const float maxAllowedDistance = 80.0f; // 最大允许分散距离
    bool forceGather = (maxDistance > maxAllowedDistance || avgDistance > 50.0f);
    
    // 检查是否可以合并
    bool canMerge = true;
    for (CloneBall* ball : m_splitBalls) {
        if (ball && !ball->isRemoved()) {
            if (ball->frameSinceLastSplit() < GoBiggerConfig::MERGE_DELAY * 60) {
                canMerge = false;
                break;
            }
        }
    }

    if (canMerge || forceGather) {
        // 朝质心聚拢
        QPointF direction = centroid - m_playerBall->pos();
        float length = QLineF(QPointF(0,0), direction).length();
        
        if (length > 5.0f) { // 如果距离质心较远才移动
            QPointF safeDirection = getSafeDirection(direction / length);
            return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
        } else {
            // 已经很接近质心了，可以执行正常策略但优先近距离食物
            return makeCoordinatedFoodHunt();
        }
    } else {
        // 冷却期间，执行协调的食物搜索（避免过度分散）
        return makeCoordinatedFoodHunt();
    }

    return makeRandomDecision(); // Fallback
}

AIAction SimpleAIPlayer::makeCoordinatedFoodHunt() {
    // 协调的食物搜索策略：分裂状态下避免球分散过度
    if (!m_playerBall || m_splitBalls.isEmpty()) {
        return makeRandomDecision();
    }
    
    // 计算当前球群的质心
    QPointF centroid(0, 0);
    float totalScore = 0.0f;
    int validBalls = 0;
    
    for (CloneBall* ball : m_splitBalls) {
        if (ball && !ball->isRemoved()) {
            QPointF pos = ball->pos();
            float score = ball->score();
            centroid += pos * score;
            totalScore += score;
            validBalls++;
        }
    }
    
    if (validBalls == 0) {
        return makeRandomDecision();
    }
    centroid /= totalScore;
    
    // 搜索附近的食物，优先选择靠近质心的食物
    auto nearbyFood = getNearbyFood(100.0f);
    
    FoodBall* bestFood = nullptr;
    float bestScore = -1.0f;
    
    for (FoodBall* food : nearbyFood) {
        if (!food || food->isRemoved()) continue;
        
        QPointF playerPos = m_playerBall->pos();
        QPointF foodPos = food->pos();
        
        float distanceToPlayer = QLineF(playerPos, foodPos).length();
        float distanceToCentroid = QLineF(centroid, foodPos).length();
        
        // 综合评分：距离当前球近 + 距离质心近 + 食物价值
        float score = food->score() / (1.0f + distanceToPlayer * 0.1f + distanceToCentroid * 0.05f);
        
        if (score > bestScore) {
            bestScore = score;
            bestFood = food;
        }
    }
    
    if (bestFood) {
        QPointF direction = bestFood->pos() - m_playerBall->pos();
        float length = QLineF(QPointF(0,0), direction).length();
        if (length > 0.1f) {
            QPointF safeDirection = getSafeDirection(direction / length);
            return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
        }
    }
    
    // 如果没有找到合适的食物，向质心缓慢移动
    QPointF direction = centroid - m_playerBall->pos();
    float length = QLineF(QPointF(0,0), direction).length();
    if (length > 10.0f) { // 只有距离质心较远时才移动
        direction = getSafeDirection(direction / length);
        // 降低移动速度，避免过快聚拢
        return AIAction(direction.x() * 0.5f, direction.y() * 0.5f, ActionType::MOVE);
    }
    
    return makeRandomDecision();
}

AIAction SimpleAIPlayer::makeAggressiveDecision() {
    // Target locking logic
    if (m_currentTarget) {
        if (m_currentTarget->isRemoved() || !m_playerBall->canEat(m_currentTarget)) {
            m_currentTarget = nullptr;
            m_targetLockFrames = 0;
        } else {
            m_targetLockFrames++;
            if (m_targetLockFrames < 10) { // Lock target for 10 frames
                QPointF direction = m_currentTarget->pos() - m_playerBall->pos();
                float length = QLineF(QPointF(0,0), direction).length();
                if (length > 0.1f) {
                    QPointF safeDirection = getSafeDirection(direction / length);
                    return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
                }
            } else {
                m_currentTarget = nullptr; // Unlock after 10 frames
                m_targetLockFrames = 0;
            }
        }
    }

    // 🔥 改进的攻击策略：综合考虑距离、速度、分数等因素
    auto nearbyPlayers = getNearbyPlayers(200.0f); // 扩大搜索范围
    
    QPointF playerPos = m_playerBall->pos();
    CloneBall* bestTarget = nullptr;
    float bestScore = -1.0f; // 最佳攻击目标评分
    
    for (auto player : nearbyPlayers) {
        if (!player || player == m_playerBall || player->isRemoved()) continue;
        
        // 🔥 只攻击其他队伍的玩家
        if (player->teamId() == m_playerBall->teamId()) continue;
        
        // 基础条件：必须能够吞噬对方
        if (!m_playerBall->canEat(player)) continue;
        
        QPointF direction = player->pos() - playerPos;
        float distance = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());
        
        if (distance < 1.0f) continue; // 避免除零
        
        // 🔥 综合评分系统
        float scoreRatio = m_playerBall->score() / std::max(player->score(), 1.0f);
        float radiusRatio = m_playerBall->radius() / std::max(player->radius(), 1.0f);
        
        // 计算目标速度（估算）
        QPointF targetVelocity = player->getVelocity();
        float targetSpeed = std::sqrt(targetVelocity.x() * targetVelocity.x() + targetVelocity.y() * targetVelocity.y());
        
        // 计算自己的相对速度优势
        QPointF myVelocity = m_playerBall->getVelocity();
        float mySpeed = std::sqrt(myVelocity.x() * myVelocity.x() + myVelocity.y() * myVelocity.y());
        float speedAdvantage = (mySpeed + 1.0f) / (targetSpeed + 1.0f);
        
        // 🔥 综合评分公式
        float attackScore = 0.0f;
        
        // 1. 分数优势（越大越好）
        attackScore += std::min(scoreRatio - 1.1f, 2.0f) * 40.0f; // 至少要有1.1倍分数优势

        // 2. 距离因素（越近越好，但太近不好分裂）
        float optimalDistance = m_playerBall->radius() * 2.5f; // 最佳攻击距离
        float distanceFactor = 1.0f - std::abs(distance - optimalDistance) / (optimalDistance * 2.0f);
        attackScore += std::max(distanceFactor, 0.0f) * 30.0f;

        // 3. 速度优势
        attackScore += (speedAdvantage - 1.0f) * 20.0f;

        // 4. 目标越小越容易攻击
        attackScore += (2.0f - radiusRatio) * 15.0f;

        // 5. 如果可以分裂攻击，额外加分
        if (m_playerBall->canSplit() && distance < m_playerBall->radius() * 3.5f) {
            attackScore += 25.0f;
        }
        
        qDebug() << "Evaluating target:" << player->ballId() 
                 << "distance:" << distance
                 << "scoreRatio:" << scoreRatio
                 << "speedAdvantage:" << speedAdvantage
                 << "attackScore:" << attackScore;
        
        if (attackScore > 50.0f && attackScore > bestScore) { // 最低攻击阈值
            bestScore = attackScore;
            bestTarget = player;
            m_currentTarget = bestTarget;
        }
    }
    
    if (bestTarget) {
        QPointF direction = bestTarget->pos() - playerPos;
        float distance = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());
        
        if (distance > 0.1f) {
            float dx = direction.x() / distance;
            float dy = direction.y() / distance;
            
            // 🔥 智能分裂攻击判断
            bool shouldSplit = false;
            if (m_playerBall->canSplit()) {
                float splitDistance = m_playerBall->radius() * 3.5f; // 分裂有效距离
                float scoreAdvantage = m_playerBall->score() / bestTarget->score();
                
                // 分裂条件：距离适中、分数优势足够、目标相对静止
                if (distance < splitDistance && 
                    distance > m_playerBall->radius() * 1.5f && 
                    scoreAdvantage > 1.5f) {
                    
                    QPointF targetVel = bestTarget->getVelocity();
                    float targetSpeed = std::sqrt(targetVel.x() * targetVel.x() + targetVel.y() * targetVel.y());
                    
                    if (targetSpeed < 30.0f) { // 目标移动较慢
                        shouldSplit = true;
                    }
                }
            }
            
            qDebug() << "Attacking target" << bestTarget->ballId() 
                     << "at distance" << distance 
                     << "split:" << shouldSplit;
            
            // 🔥 集成边界检测，确保攻击方向安全
            QPointF safeDirection = getSafeDirection(QPointF(dx, dy));
            
            return AIAction(safeDirection.x(), safeDirection.y(), shouldSplit ? ActionType::SPLIT : ActionType::MOVE);
        }
    }
    
    // 如果没有合适的攻击目标，回到食物猎手模式
    return makeFoodHunterDecision();
}

void SimpleAIPlayer::executeAction(const AIAction& action) {
    if (!m_playerBall) return;
    
    // 执行移动
    if (action.dx != 0.0f || action.dy != 0.0f) {
        QPointF targetDirection(action.dx, action.dy);
        m_playerBall->setTargetDirection(targetDirection);
    }
    
    // 执行特殊动作
    switch (action.type) {
        case ActionType::SPLIT:
            if (m_playerBall->canSplit()) {
                m_playerBall->split();
            }
            break;
            
        case ActionType::EJECT:
            if (m_playerBall->canEject()) {
                QPointF ejectDirection(action.dx, action.dy);
                if (ejectDirection.manhattanLength() > 0.1f) {
                    m_playerBall->ejectSpore(ejectDirection);
                }
            }
            break;
            
        case ActionType::MOVE:
        default:
            // 移动已经在上面处理了
            break;
    }
}

void SimpleAIPlayer::executeActionForBall(CloneBall* ball, const AIAction& action) {
    if (!ball || ball->isRemoved()) return;
    
    // 🔥 为每个球单独进行边界检测
    CloneBall* originalPlayerBall = m_playerBall;
    m_playerBall = ball; // 临时设置为当前球以进行边界检测
    
    // 执行移动 - 集成边界检测
    if (action.dx != 0.0f || action.dy != 0.0f) {
        QPointF targetDirection(action.dx, action.dy);
        QPointF safeDirection = getSafeDirection(targetDirection);
        ball->setTargetDirection(safeDirection);
    }
    
    // 恢复原始主球
    m_playerBall = originalPlayerBall;
    
    // 执行特殊动作（只对主球执行，避免多次分裂）
    if (ball == m_playerBall) {
        switch (action.type) {
            case ActionType::SPLIT:
                if (ball->canSplit()) {
                    ball->split();
                }
                break;
                
            case ActionType::EJECT:
                if (ball->canEject()) {
                    QPointF ejectDirection(action.dx, action.dy);
                    if (ejectDirection.manhattanLength() > 0.1f) {
                        ball->ejectSpore(ejectDirection);
                    }
                }
                break;
                
            case ActionType::MOVE:
            default:
                // 移动已经在上面处理了
                break;
        }
    }
}

std::vector<BaseBall*> SimpleAIPlayer::getNearbyBalls(float radius) {
    std::vector<BaseBall*> nearbyBalls;
    
    if (!m_playerBall || !m_playerBall->scene()) {
        return nearbyBalls;
    }
    
    QPointF playerPos = m_playerBall->pos();
    QRectF searchRect(playerPos.x() - radius, playerPos.y() - radius, 
                      2 * radius, 2 * radius);
    
    auto items = m_playerBall->scene()->items(searchRect);
    for (auto item : items) {
        BaseBall* ball = dynamic_cast<BaseBall*>(item);
        if (ball && ball != m_playerBall) {
            nearbyBalls.push_back(ball);
        }
    }
    
    return nearbyBalls;
}

std::vector<FoodBall*> SimpleAIPlayer::getNearbyFood(float radius) {
    std::vector<FoodBall*> nearbyFood;
    
    if (!m_playerBall || !m_playerBall->scene()) {
        return nearbyFood;
    }
    
    QPointF playerPos = m_playerBall->pos();
    QRectF searchRect(playerPos.x() - radius, playerPos.y() - radius, 
                      2 * radius, 2 * radius);
    
    auto items = m_playerBall->scene()->items(searchRect);
    for (auto item : items) {
        FoodBall* food = dynamic_cast<FoodBall*>(item);
        if (food) {
            nearbyFood.push_back(food);
        }
    }
    
    return nearbyFood;
}

std::vector<CloneBall*> SimpleAIPlayer::getNearbyPlayers(float radius) {
    std::vector<CloneBall*> nearbyPlayers;
    
    if (!m_playerBall || !m_playerBall->scene()) {
        return nearbyPlayers;
    }
    
    QPointF playerPos = m_playerBall->pos();
    QRectF searchRect(playerPos.x() - radius, playerPos.y() - radius, 
                      2 * radius, 2 * radius);
    
    auto items = m_playerBall->scene()->items(searchRect);
    for (auto item : items) {
        CloneBall* player = dynamic_cast<CloneBall*>(item);
        if (player && player != m_playerBall) {
            nearbyPlayers.push_back(player);
        }
    }
    
    return nearbyPlayers;
}

bool SimpleAIPlayer::loadAIModel(const QString& modelPath) {
    if (!m_onnxInference) {
        qWarning() << "ONNX inference not initialized";
        return false;
    }
    
    bool success = m_onnxInference->loadModel(modelPath.toStdString());
    if (success) {
        qDebug() << "AI model loaded successfully from:" << modelPath;
    } else {
        qWarning() << "Failed to load AI model from:" << modelPath;
    }
    
    return success;
}

bool SimpleAIPlayer::isModelLoaded() const {
    return m_onnxInference && m_onnxInference->isLoaded();
}

AIAction SimpleAIPlayer::makeModelBasedDecision() {
    // 如果模型未加载，回退到食物猎手策略
    if (!isModelLoaded()) {
        qDebug() << "Model not loaded, falling back to FOOD_HUNTER strategy";
        return makeFoodHunterDecision();
    }
    
    try {
        // 提取环境观察信息
        std::vector<float> observation = extractObservation();
        
        if (observation.size() != static_cast<size_t>(m_observationSize)) {
            qWarning() << "Observation size mismatch. Expected:" << m_observationSize 
                      << "Got:" << observation.size();
            return makeFoodHunterDecision();
        }
        
        // 执行模型推理
        std::vector<float> modelOutput = m_onnxInference->predict(observation);
        
        if (modelOutput.size() < 3) {
            qWarning() << "Invalid model output size:" << modelOutput.size();
            return makeFoodHunterDecision();
        }
        
        // 解析模型输出：[dx, dy, action_type]
        float dx = std::clamp(modelOutput[0], -1.0f, 1.0f);
        float dy = std::clamp(modelOutput[1], -1.0f, 1.0f);
        
        // 将action_type转换为整数并映射到动作类型
        int actionTypeInt = static_cast<int>(std::round(modelOutput[2]));
        actionTypeInt = std::clamp(actionTypeInt, 0, 2);
        
        ActionType actionType = static_cast<ActionType>(actionTypeInt);
        
        qDebug() << "Model prediction - dx:" << dx << "dy:" << dy 
                << "action_type:" << actionTypeInt;
        
        return AIAction(dx, dy, actionType);
        
    } catch (const std::exception& e) {
        qWarning() << "Model-based decision failed:" << e.what();
        return makeFoodHunterDecision();
    }
}

std::vector<float> SimpleAIPlayer::extractObservation() {
    std::vector<float> observation(m_observationSize, 0.0f);
    
    if (!m_playerBall || !m_playerBall->scene()) {
        qWarning() << "Cannot extract observation: no player ball or scene";
        return observation;
    }
    
    try {
        // 简化的特征提取 - 这是一个基础版本
        // 在实际应用中需要根据训练时的观察空间设计来实现
        
        int idx = 0;
        
        // 1. 玩家自身信息 (4个特征)
        QPointF playerPos = m_playerBall->pos();
        float playerSize = m_playerBall->radius();
        
        if (idx + 3 < m_observationSize) {
            observation[idx++] = playerPos.x() / 1000.0f; // 归一化位置
            observation[idx++] = playerPos.y() / 1000.0f;
            observation[idx++] = playerSize / 100.0f; // 归一化大小
            observation[idx++] = static_cast<float>(m_playerBall->ballId()) / 100.0f; // 玩家ID
        }
        
        // 2. 附近食物信息 (最多50个食物，每个3个特征：相对位置x,y和大小)
        auto nearbyFood = getNearbyFood(200.0f);
        int maxFood = std::min(static_cast<int>(nearbyFood.size()), 50);
        
        for (int i = 0; i < maxFood && idx + 2 < m_observationSize; ++i) {
            QPointF foodPos = nearbyFood[i]->pos();
            float relativeX = (foodPos.x() - playerPos.x()) / 200.0f; // 归一化相对位置
            float relativeY = (foodPos.y() - playerPos.y()) / 200.0f;
            float foodSize = nearbyFood[i]->radius() / 10.0f; // 归一化食物大小
            
            observation[idx++] = relativeX;
            observation[idx++] = relativeY;
            observation[idx++] = foodSize;
        }
        
        // 3. 附近其他玩家信息 (最多20个玩家，每个4个特征：相对位置x,y，大小，和威胁度)
        auto nearbyPlayers = getNearbyPlayers(150.0f);
        int maxPlayers = std::min(static_cast<int>(nearbyPlayers.size()), 20);
        
        for (int i = 0; i < maxPlayers && idx + 3 < m_observationSize; ++i) {
            QPointF otherPos = nearbyPlayers[i]->pos();
            float relativeX = (otherPos.x() - playerPos.x()) / 150.0f;
            float relativeY = (otherPos.y() - playerPos.y()) / 150.0f;
            float otherSize = nearbyPlayers[i]->radius() / 100.0f;
            float threat = (otherSize > playerSize) ? 1.0f : -1.0f; // 威胁度
            
            observation[idx++] = relativeX;
            observation[idx++] = relativeY;
            observation[idx++] = otherSize;
            observation[idx++] = threat;
        }
        
        // 4. 填充剩余特征为0（如果有的话）
        // 其余特征保持为0
        
        qDebug() << "Extracted observation with" << idx << "meaningful features out of" 
                << m_observationSize << "total features";
        
    } catch (const std::exception& e) {
        qWarning() << "Feature extraction failed:" << e.what();
    }
    
    return observation;
}

void SimpleAIPlayer::onSplitPerformed(CloneBall* originalBall, const QVector<CloneBall*>& newBalls) {
    qDebug() << "Split performed! Original ball count:" << m_splitBalls.size() 
             << "New balls:" << newBalls.size();
    
    // 移除原始球（如果存在）
    m_splitBalls.removeAll(originalBall);
    
    // 添加新的分裂球
    for (CloneBall* ball : newBalls) {
        if (ball && !m_splitBalls.contains(ball)) {
            m_splitBalls.append(ball);
            // 监听新球的销毁信号
            connect(ball, &QObject::destroyed, this, &SimpleAIPlayer::onBallDestroyed);
        }
    }
    
    qDebug() << "Now controlling" << m_splitBalls.size() << "balls";
}

void SimpleAIPlayer::onBallDestroyed(QObject* ball) {
    CloneBall* cloneBall = qobject_cast<CloneBall*>(ball);
    if (cloneBall) {
        m_splitBalls.removeAll(cloneBall);
        qDebug() << "Ball destroyed, now controlling" << m_splitBalls.size() << "balls";
        
        // 如果主球被销毁，选择新的主球
        if (cloneBall == m_playerBall && !m_splitBalls.isEmpty()) {
            m_playerBall = m_splitBalls.first();
            qDebug() << "Switched main ball to:" << m_playerBall->ballId();
        }
        
        // 如果没有球了，停止AI
        if (m_splitBalls.isEmpty()) {
            m_playerBall = nullptr;
            stopAI();
        }
    }
}

// ============ 边界检测和避障功能实现 ============

bool SimpleAIPlayer::isNearBorder(const QPointF& position, float threshold) const
{
    if (!m_playerBall) return false;
    
    const Border& border = m_playerBall->border();
    float margin = threshold + m_playerBall->radius();
    
    return (position.x() - border.minx < margin ||
            border.maxx - position.x() < margin ||
            position.y() - border.miny < margin ||
            border.maxy - position.y() < margin);
}

QPointF SimpleAIPlayer::getAvoidBorderDirection(const QPointF& position) const
{
    if (!m_playerBall) return QPointF(0, 0);
    
    const Border& border = m_playerBall->border();
    float margin = 150.0f + m_playerBall->radius();
    
    QPointF avoidDirection(0, 0);
    
    // 检查各个边界并生成避开方向
    if (position.x() - border.minx < margin) {
        avoidDirection.setX(1.0f); // 向右避开左边界
    }
    if (border.maxx - position.x() < margin) {
        avoidDirection.setX(-1.0f); // 向左避开右边界
    }
    if (position.y() - border.miny < margin) {
        avoidDirection.setY(1.0f); // 向下避开上边界
    }
    if (border.maxy - position.y() < margin) {
        avoidDirection.setY(-1.0f); // 向上避开下边界
    }
    
    // 如果在角落，取对角线方向
    if (avoidDirection.x() != 0 && avoidDirection.y() != 0) {
        avoidDirection *= 0.707f; // 归一化对角线
    }
    
    return avoidDirection;
}

QPointF SimpleAIPlayer::getSafeDirection(const QPointF& targetDirection) const
{
    if (!m_playerBall) return targetDirection;
    
    QPointF currentPos = m_playerBall->pos();
    
    // 🔥 检测是否卡住（位置变化很小）
    float distanceMoved = QLineF(currentPos, m_lastPosition).length();
    if (distanceMoved < 2.0f) {
        m_stuckFrameCount++;
    } else {
        m_stuckFrameCount = 0;
        m_lastPosition = currentPos;
    }
    
    // 🔥 更新移动方向历史（用于检测振荡）
    m_recentDirections.append(targetDirection);
    if (m_recentDirections.size() > 10) {
        m_recentDirections.removeFirst();
    }
    
    // 🔥 检测振荡：如果最近的方向频繁反向，说明在打转
    bool isOscillating = false;
    if (m_recentDirections.size() >= 6) {
        QPointF avgDirection(0, 0);
        for (const QPointF& dir : m_recentDirections) {
            avgDirection += dir;
        }
        avgDirection /= m_recentDirections.size();
        
        float avgLength = QLineF(QPointF(0,0), avgDirection).length();
        if (avgLength < 0.3f) { // 平均方向很小，说明在振荡
            isOscillating = true;
        }
    }
    
    // 🔥 如果卡住或振荡，使用紧急脱困策略
    if (m_stuckFrameCount > 5 || isOscillating) {
        qDebug() << "AI Ball" << m_playerBall->ballId() << "is stuck or oscillating, using emergency escape";
        
        // 生成随机逃脱方向
        float angle = QRandomGenerator::global()->generateDouble() * 2.0 * M_PI;
        QPointF emergencyDirection(std::cos(angle), std::sin(angle));
        
        // 确保逃脱方向远离边界
        QPointF avoidDirection = getAvoidBorderDirection(currentPos);
        if (avoidDirection.manhattanLength() > 0.1f) {
            emergencyDirection = avoidDirection + emergencyDirection * 0.3f;
            float length = QLineF(QPointF(0,0), emergencyDirection).length();
            if (length > 0.1f) {
                emergencyDirection /= length;
            }
        }
        
        m_stuckFrameCount = 0; // 重置计数
        m_recentDirections.clear(); // 清除历史
        return emergencyDirection;
    }
    
    // 🔥 边界检测和智能避障
    if (!isNearBorder(currentPos, 100.0f)) {
        return targetDirection; // 不在边界附近，直接返回
    }
    
    // 在边界附近，进行智能避障
    const Border& border = m_playerBall->border();
    float margin = 80.0f + m_playerBall->radius();
    
    QPointF safeDirection = targetDirection;
    bool needAvoidance = false;
    
    // 检查目标方向是否会导致撞墙
    QPointF futurePos = currentPos + targetDirection * 50.0f; // 预测未来位置
    
    if (futurePos.x() - border.minx < margin) {
        // 将要撞左墙，向右偏移
        if (safeDirection.x() < 0) {
            safeDirection.setX(std::abs(safeDirection.x()));
            needAvoidance = true;
        }
    }
    if (border.maxx - futurePos.x() < margin) {
        // 将要撞右墙，向左偏移
        if (safeDirection.x() > 0) {
            safeDirection.setX(-std::abs(safeDirection.x()));
            needAvoidance = true;
        }
    }
    if (futurePos.y() - border.miny < margin) {
        // 将要撞上墙，向下偏移
        if (safeDirection.y() < 0) {
            safeDirection.setY(std::abs(safeDirection.y()));
            needAvoidance = true;
        }
    }
    if (border.maxy - futurePos.y() < margin) {
        // 将要撞下墙，向上偏移
        if (safeDirection.y() > 0) {
            safeDirection.setY(-std::abs(safeDirection.y()));
            needAvoidance = true;
        }
    }
    
    if (needAvoidance) {
        m_borderCollisionCount++;
        
        // 🔥 如果连续撞墙多次，使用沿边界移动策略
        if (m_borderCollisionCount > 3) {
            QPointF wallDirection = getWallTangentDirection(currentPos);
            if (wallDirection.manhattanLength() > 0.1f) {
                qDebug() << "AI Ball" << m_playerBall->ballId() << "using wall-following strategy";
                m_borderCollisionCount = 0;
                return wallDirection;
            }
        }
        
        // 归一化避障方向
        float length = QLineF(QPointF(0,0), safeDirection).length();
        if (length > 0.1f) {
            safeDirection /= length;
        }
        
        qDebug() << "AI Ball" << m_playerBall->ballId() << "avoiding border, direction:" 
                 << targetDirection << "-> safe:" << safeDirection;
    } else {
        m_borderCollisionCount = 0; // 重置撞墙计数
    }
    
    return safeDirection;
}

QPointF SimpleAIPlayer::getWallTangentDirection(const QPointF& position) const
{
    if (!m_playerBall) return QPointF(0, 0);
    
    const Border& border = m_playerBall->border();
    float margin = 60.0f + m_playerBall->radius();
    
    // 检测最近的墙壁并返回切线方向
    bool nearLeft = (position.x() - border.minx < margin);
    bool nearRight = (border.maxx - position.x() < margin);
    bool nearTop = (position.y() - border.miny < margin);
    bool nearBottom = (border.maxy - position.y() < margin);
    
    QPointF tangentDirection(0, 0);
    
    if (nearLeft || nearRight) {
        // 靠近左右墙，沿垂直方向移动
        if (position.y() < (border.miny + border.maxy) / 2) {
            tangentDirection.setY(1.0f); // 向下
        } else {
            tangentDirection.setY(-1.0f); // 向上
        }
        
        // 如果在角落，添加远离角落的分量
        if (nearTop) {
            tangentDirection.setY(1.0f); // 强制向下
        } else if (nearBottom) {
            tangentDirection.setY(-1.0f); // 强制向上
        }
    }
    
    if (nearTop || nearBottom) {
        // 靠近上下墙，沿水平方向移动
        if (position.x() < (border.minx + border.maxx) / 2) {
            tangentDirection.setX(1.0f); // 向右
        } else {
            tangentDirection.setX(-1.0f); // 向左
        }
        
        // 如果在角落，添加远离角落的分量
        if (nearLeft) {
            tangentDirection.setX(1.0f); // 强制向右
        } else if (nearRight) {
            tangentDirection.setX(-1.0f); // 强制向左
        }
    }
    
    // 归一化方向
    float length = QLineF(QPointF(0,0), tangentDirection).length();
    if (length > 0.1f) {
        tangentDirection /= length;
    }
    
    return tangentDirection;
}

} // namespace AI
} // namespace GoBigger
