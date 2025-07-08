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
    , m_playerBall(nullptr)
    , m_decisionTimer(new QTimer(this))
    , m_aiActive(false)
    , m_decisionInterval(200) // 默认200ms决策间隔
    , m_strategy(AIStrategy::FOOD_HUNTER) // 默认食物猎手策略
    , m_currentTarget(nullptr)
    , m_targetLockFrames(0)
    , m_onnxInference(nullptr) // 🔥 暂时禁用ONNX以避免崩溃
    , m_observationSize(400) // 默认观察向量大小
    , m_stuckFrameCount(0)
    , m_lastPosition(0, 0)
    , m_borderCollisionCount(0)
    , m_lockedTarget(nullptr) // 🔥 初始化新的目标放弃机制变量
    , m_targetLockDuration(0)
    , m_huntTarget(nullptr) // 🔥 初始化追杀模式变量
    , m_huntModeFrames(0)
    , m_lastHuntTargetPos(0, 0)
    , m_shouldMerge(false) // 🔥 初始化合并相关变量
    , m_splitFrameCount(0)
    , m_mergeTargetPos(0, 0)
    , m_preferredMergeTarget(nullptr)
{
    qDebug() << "🔧 SimpleAIPlayer constructor started";
    
    // 🔥 安全检查：确保playerBall不为空且有效
    if (!playerBall) {
        qWarning() << "SimpleAIPlayer: Cannot create AI with null playerBall!";
        return;
    }
    
    qDebug() << "🔧 PlayerBall validity checked";
    
    // 🔥 验证playerBall是否已经被添加到scene中
    if (!playerBall->scene()) {
        qWarning() << "SimpleAIPlayer: PlayerBall not yet added to scene, delaying initialization";
        // 延迟初始化
        QTimer::singleShot(100, this, [this, playerBall]() {
            initializeWithPlayerBall(playerBall);
        });
        return;
    }
    
    qDebug() << "🔧 Scene check passed, proceeding with initialization";
    initializeWithPlayerBall(playerBall);
}

void SimpleAIPlayer::initializeWithPlayerBall(CloneBall* playerBall) {
    if (!playerBall) {
        qWarning() << "SimpleAIPlayer: Cannot initialize with null playerBall!";
        return;
    }
    
    m_playerBall = playerBall;
    
    // 连接定时器
    connect(m_decisionTimer, &QTimer::timeout, this, &SimpleAIPlayer::makeDecision);
    
    // 监听玩家球被销毁的信号
    connect(m_playerBall, &QObject::destroyed, this, &SimpleAIPlayer::onPlayerBallDestroyed);
    // 监听球被移除的信号
    connect(m_playerBall, &BaseBall::ballRemoved, this, &SimpleAIPlayer::onPlayerBallRemoved);
    // 监听分裂信号
    connect(m_playerBall, &CloneBall::splitPerformed, this, &SimpleAIPlayer::onSplitPerformed);
    // 🔥 监听合并信号
    connect(m_playerBall, &CloneBall::mergePerformed, this, &SimpleAIPlayer::onMergePerformed);
    
    // 初始化分裂球列表
    m_splitBalls.clear();
    m_splitBalls.append(m_playerBall);
    
    // 初始化位置记录
    m_lastPosition = m_playerBall->pos();
    
    qDebug() << "SimpleAIPlayer successfully initialized for ball:" << m_playerBall->ballId()
             << "with strategy:" << static_cast<int>(m_strategy)
             << "scene:" << (m_playerBall->scene() != nullptr);
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
    // 🔥 增强安全检查
    if (!m_aiActive || !m_playerBall) {
        return;
    }
    
    // 🔥 检查主球是否已被销毁
    if (m_playerBall->isRemoved()) {
        qDebug() << "Main player ball was removed, stopping AI";
        stopAI();
        return;
    }
    
    // 🔥 检查场景是否仍然有效
    if (!m_playerBall->scene()) {
        qWarning() << "Player ball no longer in scene, stopping AI";
        stopAI();
        return;
    }
    
    // 清理已被移除的分裂球
    m_splitBalls.removeIf([](CloneBall* ball) {
        return !ball || ball->isRemoved();
    });
    
    if (m_splitBalls.isEmpty()) {
        qDebug() << "No valid balls remaining, stopping AI";
        stopAI();
        return;
    }
    
    // 🔥 调试：打印当前AI控制的球数量和状态
    QStringList ballIds;
    for (CloneBall* ball : m_splitBalls) {
        if (ball && !ball->isRemoved()) {
            ballIds << QString::number(ball->ballId());
        }
    }
    if (ballIds.size() != m_splitBalls.size()) {
        qWarning() << "🚨 Mismatch in ball count! Valid:" << ballIds.size() << "Total:" << m_splitBalls.size();
    }
    qDebug() << "🎯 AI Decision: Controlling" << ballIds.size() << "balls:" << ballIds.join(",");
    
    // 🔥 新增：更新合并状态和计数器
    updateMergeStatus();
    
    try {
        // 🔥 修复：为每个分裂球独立决策，而不是统一行动
        for (CloneBall* ball : m_splitBalls) {
            if (!ball || ball->isRemoved()) continue;
            
            // 临时设置当前控制的球，用于各种检测和决策
            CloneBall* originalPlayerBall = m_playerBall;
            m_playerBall = ball;
            
            AIAction action;
            
            // 🔥 优先检查合并逻辑（每个球独立）
            if (shouldAttemptMerge()) {
                AIAction mergeAction = makeMergeDecision();
                if (mergeAction.dx != 0.0f || mergeAction.dy != 0.0f) {
                    // 执行合并动作
                    executeActionForBall(ball, mergeAction);
                    m_playerBall = originalPlayerBall; // 恢复主球
                    continue; // 跳过其他决策，专注合并
                }
            }
            
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
            qDebug() << "🎯 Making decision for ball" << ball->ballId() << "strategy:" << static_cast<int>(m_strategy);
            
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
            qDebug() << "🎯 Executing action for ball" << ball->ballId() << "dx:" << action.dx << "dy:" << action.dy << "type:" << static_cast<int>(action.type);
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
    // 🔥 增强目标锁定逻辑，减少频繁切换目标导致的打转
    if (m_currentTarget) {
        if (m_currentTarget->isRemoved() || !m_playerBall->canEat(m_currentTarget)) {
            m_currentTarget = nullptr;
            m_targetLockFrames = 0;
        } else {
            m_targetLockFrames++;
            // 🔥 延长目标锁定时间，减少切换频率
            if (m_targetLockFrames < 15) { // 从10增加到15帧
                QPointF direction = m_currentTarget->pos() - m_playerBall->pos();
                float length = QLineF(QPointF(0,0), direction).length();
                if (length > 0.1f) {
                    QPointF safeDirection = getSafeDirection(direction / length);
                    return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
                }
            } else {
                // 🔥 目标锁定超时后，检查是否应该继续追求该目标
                QPointF direction = m_currentTarget->pos() - m_playerBall->pos();
                float distance = QLineF(QPointF(0,0), direction).length();
                
                // 如果目标很近，继续追求；否则解锁
                if (distance < 80.0f) {
                    m_targetLockFrames = 10; // 重置为较小值，继续锁定
                    QPointF safeDirection = getSafeDirection(direction / distance);
                    return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
                } else {
                    m_currentTarget = nullptr; // 目标太远，解锁
                    m_targetLockFrames = 0;
                }
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
    }
    
    // 2. 紧急威胁处理 - 高威胁时分裂逃跑
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
            } else if (distance < playerRadius + ball->radius() + 30.0f) { // 增加安全距离
                // 🔥 优化：更智能的荆棘球避障，使用切线方向避免打转
                QPointF awayDirection = playerPos - ball->pos();
                float awayLength = QLineF(QPointF(0,0), awayDirection).length();
                
                if (awayLength > 0.1f) {
                    awayDirection /= awayLength;
                    
                    // 🔥 新增：计算切线方向，避免直线逃离导致的反复震荡
                    QPointF tangent(-awayDirection.y(), awayDirection.x()); // 垂直方向
                    
                    // 选择更好的切线方向（朝向更多食物的方向）
                    float leftScore = 0, rightScore = 0;
                    for (auto food : nearbyFood) {
                        QPointF foodDir = food->pos() - playerPos;
                        float leftDot = QPointF::dotProduct(foodDir, tangent);
                        float rightDot = QPointF::dotProduct(foodDir, -tangent);
                        if (leftDot > 0) leftScore += food->score();
                        if (rightDot > 0) rightScore += food->score();
                    }
                    
                    QPointF finalDirection;
                    if (rightScore > leftScore) {
                        finalDirection = tangent * 0.8f + awayDirection * 0.2f; // 主要切线+少量远离
                    } else {
                        finalDirection = -tangent * 0.8f + awayDirection * 0.2f;
                    }
                    
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
        
        // 🔥 优化食物选择算法，减少频繁切换目标 + 目标放弃机制
        FoodBall* bestFood = nullptr;
        float bestScore = -1.0f;
        
        // 如果当前有目标且仍然有效，给予额外加分避免频繁切换
        float currentTargetBonus = 0.0f;
        if (m_currentTarget) {
            for (auto food : nearbyFood) {
                if (food == m_currentTarget) {
                    currentTargetBonus = 2.0f; // 给当前目标额外加分
                    break;
                }
            }
        }
        
        // 🔥 新增：检查目标放弃机制
        auto cleanupAbandonedTargets = [this, &nearbyFood]() {
            // 清理已不存在的目标
            QSet<int> currentFoodIds;
            for (auto food : nearbyFood) {
                currentFoodIds.insert(food->ballId());
            }
            
            // 移除不存在的目标
            for (auto it = m_failedTargetAttempts.begin(); it != m_failedTargetAttempts.end();) {
                if (!currentFoodIds.contains(it.key())) {
                    it = m_failedTargetAttempts.erase(it);
                } else {
                    ++it;
                }
            }
            
            for (auto it = m_abandonedTargets.begin(); it != m_abandonedTargets.end();) {
                if (!currentFoodIds.contains(*it)) {
                    it = m_abandonedTargets.erase(it);
                } else {
                    ++it;
                }
            }
        };
        cleanupAbandonedTargets();
        
        for (auto food : nearbyFood) {
            QPointF foodPos = food->pos();
            float distance = QLineF(foodPos, playerPos).length();
            int foodId = food->ballId();
            
            // 🔥 跳过已放弃的目标
            if (m_abandonedTargets.contains(foodId)) {
                continue;
            }
            
            // 🔥 检查是否应该放弃此目标
            if (m_failedTargetAttempts.contains(foodId)) {
                int attempts = m_failedTargetAttempts[foodId];
                
                // 如果尝试次数过多且距离较远，放弃目标
                if (attempts > 8 && distance > 50.0f) {
                    qDebug() << "Abandoning target food" << foodId << "after" << attempts << "failed attempts";
                    m_abandonedTargets.insert(foodId);
                    m_failedTargetAttempts.remove(foodId);
                    if (m_currentTarget == food) {
                        m_currentTarget = nullptr;
                        m_targetLockFrames = 0;
                    }
                    continue;
                }
                
                // 尝试次数较少但距离很远，减少评分
                if (attempts > 3 && distance > 80.0f) {
                    // 减少评分，使其不太可能被选中
                    continue;
                }
            }
            
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
                // 评分：食物价值/距离 + 密度加成 + 当前目标加成
                float localDensity = 0;
                for (auto otherFood : nearbyFood) {
                    if (QLineF(otherFood->pos(), foodPos).length() < 40.0f) {
                        localDensity += 1.0f;
                    }
                }
                
                float score = (food->score() / (distance + 1.0f)) * (1.0f + localDensity * 0.2f);
                
                // 🔥 当前目标加成，减少频繁切换
                if (food == m_currentTarget) {
                    score += currentTargetBonus;
                }
                
                if (score > bestScore) {
                    bestScore = score;
                    bestFood = food;
                }
            }
        }
        
        if (bestFood) {
            m_currentTarget = bestFood;
            int foodId = bestFood->ballId();
            
            // 🔥 新增：跟踪目标尝试次数
            if (m_lockedTarget != bestFood) {
                // 切换到新目标，重置锁定时间
                m_lockedTarget = bestFood;
                m_targetLockDuration = 0;
            } else {
                // 继续追求同一目标，增加锁定时间
                m_targetLockDuration++;
                
                // 🔥 检查是否长时间无法获取目标
                if (m_targetLockDuration > 30) { // 30帧 = 约1.5秒
                    float distance = QLineF(bestFood->pos(), playerPos).length();
                    
                    // 如果距离没有显著减少，增加失败计数
                    if (distance > 60.0f) {
                        m_failedTargetAttempts[foodId]++;
                        qDebug() << "Target food" << foodId << "seems unreachable, failed attempts:" 
                                 << m_failedTargetAttempts[foodId];
                        
                        // 重置锁定，尝试其他目标
                        m_lockedTarget = nullptr;
                        m_targetLockDuration = 0;
                        m_currentTarget = nullptr;
                        m_targetLockFrames = 0;
                        
                        // 如果失败次数过多，临时放弃
                        if (m_failedTargetAttempts[foodId] >= 5) {
                            qDebug() << "Temporarily abandoning unreachable target" << foodId;
                            return AIAction(0, 0, ActionType::MOVE); // 停止移动，重新评估
                        }
                    }
                }
            }
            
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
        // 🔥 改进探索策略：朝向更大的食物聚集区域移动，而不是随机移动
        QVector2D explorationDirection(0, 0);
        float totalWeight = 0.0f;
        
        for (auto food : nearbyFood) {
            QPointF foodPos = food->pos();
            QPointF direction = foodPos - playerPos;
            float distance = QLineF(QPointF(0,0), direction).length();
            
            if (distance > 0.1f) {
                // 距离越近，权重越高；食物越大，权重越高
                float weight = (food->score() * 100.0f) / (distance + 10.0f);
                explorationDirection += QVector2D(direction / distance) * weight;
                totalWeight += weight;
            }
        }
        
        if (totalWeight > 0.1f) {
            explorationDirection = explorationDirection.normalized();
            QPointF safeDirection = getSafeDirection(QPointF(explorationDirection.x(), explorationDirection.y()));
            return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
        }
    }
    
    // 6. 最后选择：向中心移动而不是完全随机
    QPointF toCenter = QPointF(0, 0) - playerPos;
    float centerDistance = QLineF(QPointF(0,0), toCenter).length();
    if (centerDistance > 100.0f) { // 如果距离中心较远
        toCenter /= centerDistance;
        QPointF safeDirection = getSafeDirection(toCenter);
        return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
    }
    
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
    QPointF playerPos = m_playerBall->pos();
    
    // 🔥 锁定追杀模式：一旦锁定目标就不再吃食物，专注追杀
    if (m_huntTarget && !m_huntTarget->isRemoved()) {
        float distance = QLineF(m_huntTarget->pos(), playerPos).length();
        float maxHuntDistance = 300.0f; // 最大追杀距离
        
        // 检查是否还能继续追杀
        bool canStillHunt = m_playerBall->canEat(m_huntTarget) && distance < maxHuntDistance;
        
        if (canStillHunt) {
            m_huntModeFrames++;
            qDebug() << "🎯 HUNT MODE: Chasing target" << m_huntTarget->ballId() 
                     << "for" << m_huntModeFrames << "frames, distance:" << distance;
            
            QPointF direction = m_huntTarget->pos() - playerPos;
            float length = QLineF(QPointF(0,0), direction).length();
            
            if (length > 0.1f) {
                direction /= length;
                
                // 🔥 预测目标移动并拦截
                QPointF targetVel = m_huntTarget->getVelocity();
                float targetSpeed = QLineF(QPointF(0,0), targetVel).length();
                
                if (targetSpeed > 5.0f) {
                    // 计算拦截位置
                    float myMaxSpeed = 20.0f; // 使用CloneBall默认最大速度
                    float timeToIntercept = distance / (myMaxSpeed + 1.0f);
                    QPointF predictedPos = m_huntTarget->pos() + targetVel * timeToIntercept;
                    
                    QPointF interceptDirection = predictedPos - playerPos;
                    float interceptLength = QLineF(QPointF(0,0), interceptDirection).length();
                    if (interceptLength > 0.1f) {
                        direction = interceptDirection / interceptLength;
                    }
                }
                
                // 🔥 平衡的分裂攻击判断
                bool shouldSplit = false;
                if (m_playerBall->canSplit() && 
                    distance < m_playerBall->radius() * 3.5f &&  // 稍微扩大分裂距离
                    distance > m_playerBall->radius() * 1.2f) {  // 降低最小分裂距离
                    float scoreAdvantage = m_playerBall->score() / std::max(m_huntTarget->score(), 1.0f);
                    
                    // 🔥 更平衡的分裂条件：有优势就可以分裂，多吃总是好的
                    if (scoreAdvantage > 1.4f && m_huntModeFrames > 5) { // 降低分裂门槛，减少等待时间
                        QPointF targetVel = m_huntTarget->getVelocity();
                        float targetSpeed = QLineF(QPointF(0,0), targetVel).length();
                        
                        // 放宽速度条件，或者有足够大的优势时忽略速度
                        if (targetSpeed < 20.0f || scoreAdvantage > 2.0f) {
                            shouldSplit = true;
                        }
                    }
                }
                
                QPointF safeDirection = getSafeDirection(direction);
                return AIAction(safeDirection.x(), safeDirection.y(), 
                               shouldSplit ? ActionType::SPLIT : ActionType::MOVE);
            }
        } else {
            // 追杀失败，退出追杀模式
            qDebug() << "🎯 Hunt mode ended for target" << (m_huntTarget ? m_huntTarget->ballId() : -1) 
                     << "- reason:" << (m_huntTarget->isRemoved() ? "removed" : 
                                     !m_playerBall->canEat(m_huntTarget) ? "can't eat" : "too far");
            m_huntTarget = nullptr;
            m_huntModeFrames = 0;
        }
    }
    
    // 🔥 寻找新的追杀目标（更激进的条件）
    if (!m_huntTarget) {
        auto nearbyPlayers = getNearbyPlayers(250.0f); // 扩大搜索范围
        
        CloneBall* bestHuntTarget = nullptr;
        float bestHuntScore = -1.0f;
        
        for (auto player : nearbyPlayers) {
            if (!player || player == m_playerBall || player->isRemoved()) continue;
            if (player->teamId() == m_playerBall->teamId()) continue; // 不攻击队友
            
            // 🔥 追杀条件调整：更加理性
            if (!m_playerBall->canEat(player)) continue;
            
            float distance = QLineF(player->pos(), playerPos).length();
            float scoreAdvantage = m_playerBall->score() / std::max(player->score(), 1.0f);
            
            // 🔥 追杀评分：更保守的条件
            float huntScore = 0.0f;
            
            // 分数优势（提高门槛，更保守）
            if (scoreAdvantage > 1.5f) { // 从1.2提高到1.5
                huntScore += (scoreAdvantage - 1.5f) * 40.0f; // 调整基数
            }
            
            // 距离因素（偏向较近的目标）
            if (distance < 180.0f) { // 扩大考虑范围
                huntScore += (180.0f - distance) / 180.0f * 30.0f;
            }
            
            // 🔥 去掉目标大小限制，多吃总是好的！任何能吃的都可以追
            // 但是给小目标额外加分，因为更容易成功
            float targetRadius = player->radius();
            float myRadius = m_playerBall->radius();
            if (targetRadius < myRadius * 0.8f) { // 比自己小的目标都给加分
                huntScore += 20.0f;
            }
            
            // 🔥 目标速度（越慢越容易追杀）
            QPointF targetVel = player->getVelocity();
            float targetSpeed = QLineF(QPointF(0,0), targetVel).length();
            if (targetSpeed < 30.0f) { // 放宽速度门槛
                huntScore += 15.0f;
            }
            
            // 🔥 分裂状态检测：给分裂目标更多加分
            if (player->radius() < m_playerBall->radius() * 0.7f && scoreAdvantage > 1.2f) {
                huntScore += 40.0f; // 提高分裂目标的吸引力
                qDebug() << "🎯 Found potential split target:" << player->ballId() 
                         << "score advantage:" << scoreAdvantage;
            }
            
            // 🔥 安全检查：附近有威胁时降低追杀倾向
            auto nearbyThreats = getNearbyPlayers(120.0f);
            int threatCount = 0;
            for (auto threat : nearbyThreats) {
                if (threat != m_playerBall && threat != player && 
                    threat->teamId() != m_playerBall->teamId() &&
                    threat->score() > m_playerBall->score() * 0.9f) {
                    threatCount++;
                }
            }
            if (threatCount > 0) {
                huntScore -= threatCount * 20.0f; // 有威胁时大幅降低追杀倾向
            }
            
            if (huntScore > 65.0f && huntScore > bestHuntScore) { // 降低追杀门槛，更容易追杀
                bestHuntScore = huntScore;
                bestHuntTarget = player;
            }
        }
        
        // 🔥 进入追杀模式
        if (bestHuntTarget) {
            m_huntTarget = bestHuntTarget;
            m_huntModeFrames = 0;
            m_lastHuntTargetPos = bestHuntTarget->pos();
            
            qDebug() << "🎯 ENTERING HUNT MODE for target" << bestHuntTarget->ballId() 
                     << "with score:" << bestHuntScore;
            
            // 立即开始追杀
            QPointF direction = bestHuntTarget->pos() - playerPos;
            float length = QLineF(QPointF(0,0), direction).length();
            if (length > 0.1f) {
                QPointF safeDirection = getSafeDirection(direction / length);
                return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
            }
        }
    }
    
    // 🔥 传统攻击逻辑（作为备选方案）
    if (m_currentTarget) {
        if (m_currentTarget->isRemoved() || !m_playerBall->canEat(m_currentTarget)) {
            m_currentTarget = nullptr;
            m_targetLockFrames = 0;
        } else {
            m_targetLockFrames++;
            if (m_targetLockFrames < 15) { // 延长锁定时间
                QPointF direction = m_currentTarget->pos() - m_playerBall->pos();
                float length = QLineF(QPointF(0,0), direction).length();
                if (length > 0.1f) {
                    QPointF safeDirection = getSafeDirection(direction / length);
                    return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
                }
            } else {
                m_currentTarget = nullptr;
                m_targetLockFrames = 0;
            }
        }
    }
    
    // 🔥 如果没有找到追杀目标，寻找普通攻击目标
    auto nearbyPlayers = getNearbyPlayers(180.0f);
    CloneBall* bestTarget = nullptr;
    float bestScore = -1.0f;
    
    for (auto player : nearbyPlayers) {
        if (!player || player == m_playerBall || player->isRemoved()) continue;
        if (player->teamId() == m_playerBall->teamId()) continue;
        if (!m_playerBall->canEat(player)) continue;
        
        float distance = QLineF(player->pos(), playerPos).length();
        float scoreAdvantage = m_playerBall->score() / std::max(player->score(), 1.0f);
        
        float attackScore = (scoreAdvantage - 1.0f) * 30.0f + (180.0f - distance) / 180.0f * 20.0f;
        
        if (attackScore > 20.0f && attackScore > bestScore) {
            bestScore = attackScore;
            bestTarget = player;
        }
    }
    
    if (bestTarget) {
        m_currentTarget = bestTarget;
        QPointF direction = bestTarget->pos() - playerPos;
        float length = QLineF(QPointF(0,0), direction).length();
        if (length > 0.1f) {
            QPointF safeDirection = getSafeDirection(direction / length);
            return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
        }
    }
    
    // 🔥 没有攻击目标时，回到食物猎手模式（但保持攻击性）
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
    if (!ball || ball->isRemoved()) {
        qWarning() << "🚨 executeActionForBall: Ball is null or removed!";
        return;
    }
    
    qDebug() << "🎮 Executing action for ball" << ball->ballId() 
             << "dx:" << action.dx << "dy:" << action.dy << "type:" << static_cast<int>(action.type);
    
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

std::vector<FoodBall*> SimpleAIPlayer::getNearbyFood(float radius) const {
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

std::vector<CloneBall*> SimpleAIPlayer::getNearbyPlayers(float radius) const {
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
    // 🔥 ONNX暂时禁用
    qDebug() << "ONNX disabled for safety, model loading skipped:" << modelPath;
    return false;
    
    /* 原ONNX代码暂时注释
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
    */
}

bool SimpleAIPlayer::isModelLoaded() const {
    // 🔥 ONNX暂时禁用
    return false;
    // return m_onnxInference && m_onnxInference->isLoaded();
}

AIAction SimpleAIPlayer::makeModelBasedDecision() {
    // 🔥 ONNX暂时禁用，直接回退到食物猎手策略
    qDebug() << "ONNX disabled for safety, falling back to FOOD_HUNTER strategy";
    return makeFoodHunterDecision();
    
    /* 原ONNX代码暂时注释
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
    */
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
    qDebug() << "🔄 Split performed! Original ball count:" << m_splitBalls.size() 
             << "New balls:" << newBalls.size();
    
    // 移除原始球（如果存在）
    m_splitBalls.removeAll(originalBall);
    
    // 添加新的分裂球
    for (CloneBall* ball : newBalls) {
        if (ball && !m_splitBalls.contains(ball)) {
            m_splitBalls.append(ball);
            
            // 🔥 重要：为每个新球连接所有必要的信号
            connect(ball, &QObject::destroyed, this, &SimpleAIPlayer::onBallDestroyed);
            connect(ball, &CloneBall::splitPerformed, this, &SimpleAIPlayer::onSplitPerformed);
            connect(ball, &CloneBall::mergePerformed, this, &SimpleAIPlayer::onMergePerformed); // 🔥 新增：连接合并信号
            
            qDebug() << "🔄 Added ball" << ball->ballId() << "to AI control";
        }
    }
    
    // 🔥 确保我们有主球
    if (m_splitBalls.isEmpty()) {
        qWarning() << "🚨 No balls remaining after split!";
        stopAI();
        return;
    }
    
    // 🔥 如果主球不在列表中，选择第一个球作为主球
    if (!m_splitBalls.contains(m_playerBall)) {
        m_playerBall = m_splitBalls.first();
        qDebug() << "🔄 Updated main ball to:" << m_playerBall->ballId();
    }
    
    qDebug() << "🔄 Now controlling" << m_splitBalls.size() << "balls";
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
    
    // 🔥 更严格的卡住检测：降低移动阈值，更快识别卡住状态
    float distanceMoved = QLineF(currentPos, m_lastPosition).length();
    if (distanceMoved < 1.0f) { // 从2.0f降低到1.0f
        m_stuckFrameCount++;
    } else {
        m_stuckFrameCount = 0;
        m_lastPosition = currentPos;
    }
    
    // 🔥 更新移动方向历史（用于检测振荡）
    m_recentDirections.append(targetDirection);
    if (m_recentDirections.size() > 8) { // 从10降低到8，更快检测振荡
        m_recentDirections.removeFirst();
    }
    
    // 🔥 更敏感的振荡检测
    bool isOscillating = false;
    if (m_recentDirections.size() >= 6) { // 从6降低，更快检测
        QPointF avgDirection(0, 0);
        float totalLength = 0.0f;
        
        for (const QPointF& dir : m_recentDirections) {
            avgDirection += dir;
            totalLength += QLineF(QPointF(0,0), dir).length();
        }
        avgDirection /= m_recentDirections.size();
        
        float avgLength = QLineF(QPointF(0,0), avgDirection).length();
        float avgIndividualLength = totalLength / m_recentDirections.size();
        
        // 如果平均方向很小，但个体方向有长度，说明在振荡
        if (avgLength < 0.4f && avgIndividualLength > 0.3f) { // 提高敏感度
            isOscillating = true;
        }
        
        // 🔥 新增：检测方向频繁反转（打转的另一种形式）
        int reverseCount = 0;
        for (int i = 1; i < m_recentDirections.size(); ++i) {
            QPointF prev = m_recentDirections[i-1];
            QPointF curr = m_recentDirections[i];
            float dot = QPointF::dotProduct(prev, curr);
            if (dot < -0.5f) { // 方向几乎相反
                reverseCount++;
            }
        }
        if (reverseCount >= 3) { // 如果有3次以上的方向反转
            isOscillating = true;
        }
    }
    
    // 🔥 更快的脱困触发：从5帧降低到3帧
    if (m_stuckFrameCount > 3 || isOscillating) {
        qDebug() << "AI Ball" << m_playerBall->ballId() << "is stuck or oscillating (stuck:" 
                 << m_stuckFrameCount << "oscillating:" << isOscillating << "), using emergency escape";
        
        // 🔥 改进的脱困策略：尝试多个方向
        static int escapeAttempt = 0;
        escapeAttempt++;
        
        QPointF emergencyDirection;
        
        // 尝试不同的脱困方向
        switch (escapeAttempt % 4) {
            case 0: {
                // 随机方向
                float angle = QRandomGenerator::global()->generateDouble() * 2.0 * M_PI;
                emergencyDirection = QPointF(std::cos(angle), std::sin(angle));
                break;
            }
            case 1: {
                // 向中心移动
                QPointF center(0, 0);
                emergencyDirection = center - currentPos;
                float length = QLineF(QPointF(0,0), emergencyDirection).length();
                if (length > 0.1f) emergencyDirection /= length;
                break;
            }
            case 2: {
                // 向最近的食物方向移动
                auto nearbyFood = getNearbyFood(150.0f);
                if (!nearbyFood.empty()) {
                    QPointF closestFood = nearbyFood[0]->pos();
                    emergencyDirection = closestFood - currentPos;
                    float length = QLineF(QPointF(0,0), emergencyDirection).length();
                    if (length > 0.1f) emergencyDirection /= length;
                } else {
                    emergencyDirection = QPointF(1, 0); // 默认向右
                }
                break;
            }
            case 3: {
                // 垂直于最近的边界移动
                const Border& border = m_playerBall->border();
                QPointF borderEscape(0, 0);
                if (currentPos.x() - border.minx < 200) borderEscape.setX(1);
                if (border.maxx - currentPos.x() < 200) borderEscape.setX(-1);
                if (currentPos.y() - border.miny < 200) borderEscape.setY(1);
                if (border.maxy - currentPos.y() < 200) borderEscape.setY(-1);
                
                if (borderEscape.manhattanLength() > 0.1f) {
                    emergencyDirection = borderEscape;
                } else {
                    emergencyDirection = QPointF(0, 1); // 默认向下
                }
                break;
            }
        }
        
        // 确保脱困方向远离边界
        QPointF avoidDirection = getAvoidBorderDirection(currentPos);
        if (avoidDirection.manhattanLength() > 0.1f) {
            emergencyDirection = avoidDirection * 0.7f + emergencyDirection * 0.3f;
            float length = QLineF(QPointF(0,0), emergencyDirection).length();
            if (length > 0.1f) {
                emergencyDirection /= length;
            }
        }
        
        m_stuckFrameCount = 0; // 重置计数
        m_recentDirections.clear(); // 清除历史
        
        qDebug() << "Emergency escape direction:" << emergencyDirection;
        return emergencyDirection;
    }
    
    // 🔥 边界检测和智能避障
    if (!isNearBorder(currentPos, 100.0f)) {
        return targetDirection; // 不在边界附近，直接返回
    }
    
    // 在边界附近，进行智能避障
    const Border& border = m_playerBall->border();
    float margin = 60.0f + m_playerBall->radius(); // 减小边界缓冲区，避免过早转向
    
    QPointF safeDirection = targetDirection;
    bool needAvoidance = false;
    
    // 检查目标方向是否会导致撞墙 - 预测更远的未来位置
    QPointF futurePos = currentPos + targetDirection * 40.0f; // 从50.0f减少到40.0f
    
    if (futurePos.x() - border.minx < margin) {
        // 将要撞左墙，向右偏移
        if (safeDirection.x() < 0) {
            safeDirection.setX(std::abs(safeDirection.x()) * 0.8f); // 减少反弹强度
            needAvoidance = true;
        }
    }
    if (border.maxx - futurePos.x() < margin) {
        // 将要撞右墙，向左偏移
        if (safeDirection.x() > 0) {
            safeDirection.setX(-std::abs(safeDirection.x()) * 0.8f);
            needAvoidance = true;
        }
    }
    if (futurePos.y() - border.miny < margin) {
        // 将要撞上墙，向下偏移
        if (safeDirection.y() < 0) {
            safeDirection.setY(std::abs(safeDirection.y()) * 0.8f);
            needAvoidance = true;
        }
    }
    if (border.maxy - futurePos.y() < margin) {
        // 将要撞下墙，向上偏移
        if (safeDirection.y() > 0) {
            safeDirection.setY(-std::abs(safeDirection.y()) * 0.8f);
            needAvoidance = true;
        }
    }
    
    if (needAvoidance) {
        m_borderCollisionCount++;
        
        // 🔥 降低沿墙移动的触发阈值，更早使用
        if (m_borderCollisionCount > 2) { // 从3降低到2
            QPointF wallDirection = getWallTangentDirection(currentPos);
            if (wallDirection.manhattanLength() > 0.1f) {
                qDebug() << "AI Ball" << m_playerBall->ballId() << "using wall-following strategy (attempt" << m_borderCollisionCount << ")";
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

// 🔥 ============ 分裂球合并管理实现 ============

std::vector<CloneBall*> SimpleAIPlayer::getAllMyBalls() const {
    std::vector<CloneBall*> myBalls;
    
    // 🔥 优先使用m_splitBalls列表，这是AI实际控制的球
    for (CloneBall* ball : m_splitBalls) {
        if (ball && !ball->isRemoved()) {
            myBalls.push_back(ball);
        }
    }
    
    // 🔥 如果splitBalls为空，回退到场景搜索
    if (myBalls.empty() && m_playerBall && m_playerBall->scene()) {
        auto items = m_playerBall->scene()->items();
        for (auto item : items) {
            CloneBall* ball = dynamic_cast<CloneBall*>(item);
            if (ball && !ball->isRemoved() && 
                ball->teamId() == m_playerBall->teamId() && 
                ball->playerId() == m_playerBall->playerId()) {
                myBalls.push_back(ball);
            }
        }
    }
    
    return myBalls;
}

bool SimpleAIPlayer::shouldAttemptMerge() const {
    auto myBalls = getAllMyBalls();
    
    // 只有多于一个球时才考虑合并
    if (myBalls.size() <= 1) {
        m_shouldMerge = false;
        return false;
    }
    
    // 检查是否有球可以合并
    bool hasValidMergeTarget = false;
    for (auto ball1 : myBalls) {
        for (auto ball2 : myBalls) {
            if (ball1 != ball2 && ball1->canMergeWith(ball2)) {
                hasValidMergeTarget = true;
                break;
            }
        }
        if (hasValidMergeTarget) break;
    }
    
    if (!hasValidMergeTarget) {
        return false;
    }
    
    // 🔥 合并触发条件
    bool shouldMerge = false;
    
    // 1. 追杀任务完成：没有追杀目标或追杀目标已消失
    if (!m_huntTarget || m_huntTarget->isRemoved()) {
        shouldMerge = true;
        qDebug() << "🔗 Should merge: Hunt target completed/lost";
    }
    
    // 2. 分裂时间过长：超过15秒
    if (m_splitFrameCount > 15 * 60) { // 15秒 * 60帧
        shouldMerge = true;
        qDebug() << "🔗 Should merge: Split too long (" << m_splitFrameCount/60 << "s)";
    }
    
    // 3. 安全环境：附近没有威胁
    auto nearbyPlayers = getNearbyPlayers(200.0f);
    bool hasThreat = false;
    for (auto player : nearbyPlayers) {
        if (player->teamId() != m_playerBall->teamId() && 
            player->score() > m_playerBall->score() * 0.8f) {
            hasThreat = true;
            break;
        }
    }
    if (!hasThreat && m_splitFrameCount > 5 * 60) { // 安全环境下5秒后就可以合并
        shouldMerge = true;
        qDebug() << "🔗 Should merge: Safe environment";
    }
    
    // 4. 分裂球过于分散：最远距离超过400像素
    if (myBalls.size() > 1) {
        float maxDistance = 0;
        for (auto ball1 : myBalls) {
            for (auto ball2 : myBalls) {
                if (ball1 != ball2) {
                    float dist = QLineF(ball1->pos(), ball2->pos()).length();
                    maxDistance = std::max(maxDistance, dist);
                }
            }
        }
        if (maxDistance > 400.0f) {
            shouldMerge = true;
            qDebug() << "🔗 Should merge: Balls too scattered (" << maxDistance << "px)";
        }
    }
    
    m_shouldMerge = shouldMerge;
    return shouldMerge;
}

CloneBall* SimpleAIPlayer::findBestMergeTarget() const {
    auto myBalls = getAllMyBalls();
    
    if (myBalls.size() <= 1) {
        return nullptr;
    }
    
    CloneBall* bestTarget = nullptr;
    float bestScore = -1.0f;
    QPointF currentPos = m_playerBall->pos();
    
    for (auto ball : myBalls) {
        if (ball == m_playerBall || !m_playerBall->canMergeWith(ball)) {
            continue;
        }
        
        float distance = QLineF(currentPos, ball->pos()).length();
        float ballScore = ball->score();
        
        // 评分：球越大越好，距离越近越好
        float score = ballScore / (distance + 10.0f);
        
        // 优先选择最大的球
        if (ballScore > m_playerBall->score()) {
            score += 100.0f;
        }
        
        if (score > bestScore) {
            bestScore = score;
            bestTarget = ball;
        }
    }
    
    return bestTarget;
}

AIAction SimpleAIPlayer::makeMergeDecision() {
    CloneBall* mergeTarget = findBestMergeTarget();
    
    if (!mergeTarget) {
        m_shouldMerge = false;
        return AIAction(0, 0, ActionType::MOVE);
    }
    
    m_preferredMergeTarget = mergeTarget;
    QPointF targetPos = mergeTarget->pos();
    QPointF currentPos = m_playerBall->pos();
    
    QPointF direction = targetPos - currentPos;
    float distance = QLineF(QPointF(0,0), direction).length();
    
    if (distance < 0.1f) {
        return AIAction(0, 0, ActionType::MOVE);
    }
    
    direction /= distance;
    
    // 🔥 安全检查：确保合并路径安全
    QPointF safeDirection = getSafeDirection(direction);
    
    qDebug() << "🔗 Merging: Moving towards ball at" << targetPos.x() << targetPos.y() 
             << "distance:" << distance;
    
    return AIAction(safeDirection.x(), safeDirection.y(), ActionType::MOVE);
}

void SimpleAIPlayer::updateMergeStatus() {
    auto myBalls = getAllMyBalls();
    
    // 更新分裂计数器
    if (myBalls.size() > 1) {
        m_splitFrameCount++;
    } else {
        m_splitFrameCount = 0;
        m_shouldMerge = false;
        m_preferredMergeTarget = nullptr;
    }
    
    // 清理无效的合并目标
    if (m_preferredMergeTarget && 
        (m_preferredMergeTarget->isRemoved() || 
         !m_playerBall->canMergeWith(m_preferredMergeTarget))) {
        m_preferredMergeTarget = nullptr;
    }
}

void SimpleAIPlayer::onMergePerformed(CloneBall* survivingBall, CloneBall* mergedBall) {
    qDebug() << "🔗 Merge performed! Surviving ball:" << survivingBall->ballId() 
             << "Merged ball:" << mergedBall->ballId();
    
    // 移除被合并的球
    m_splitBalls.removeAll(mergedBall);
    
    // 确保合并后的球在列表中并重新连接信号
    if (!m_splitBalls.contains(survivingBall)) {
        m_splitBalls.append(survivingBall);
        qDebug() << "🔗 Added surviving ball" << survivingBall->ballId() << "to AI control";
    }
    
    // 🔥 重要：重新连接合并后球的所有信号，确保AI持续控制
    disconnect(survivingBall, nullptr, this, nullptr); // 先断开所有连接
    connect(survivingBall, &QObject::destroyed, this, &SimpleAIPlayer::onBallDestroyed);
    connect(survivingBall, &CloneBall::splitPerformed, this, &SimpleAIPlayer::onSplitPerformed);
    connect(survivingBall, &CloneBall::mergePerformed, this, &SimpleAIPlayer::onMergePerformed);
    
    // 🔥 如果主球被合并了，更新主球引用
    if (m_playerBall == mergedBall) {
        m_playerBall = survivingBall;
        qDebug() << "🔗 Updated main ball to surviving ball:" << survivingBall->ballId();
    }
    
    // 清理合并目标引用
    if (m_preferredMergeTarget == mergedBall) {
        m_preferredMergeTarget = nullptr;
    }
    
    qDebug() << "🔗 Now controlling" << m_splitBalls.size() << "balls after merge";
}

} // namespace AI
} // namespace GoBigger
