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
    , m_onnxInference(std::make_unique<ONNXInference>())
    , m_observationSize(400) // 默认观察向量大小
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
        // 🔥 优化：多球协调控制 - 防止球分散太远
        if (m_splitBalls.size() > 1) {
            // 计算所有存活球的质心
            QPointF centerOfMass(0, 0);
            float totalScore = 0.0f;
            int aliveCount = 0;
            
            for (CloneBall* ball : m_splitBalls) {
                if (ball && !ball->isRemoved()) {
                    QPointF pos = ball->pos();
                    float score = ball->score();
                    centerOfMass += pos * score; // 按分数加权
                    totalScore += score;
                    aliveCount++;
                }
            }
            
            if (aliveCount > 1 && totalScore > 0) {
                centerOfMass /= totalScore;
                
                // 检查是否有球距离质心太远（可能出视野）
                const float maxDistance = 150.0f; // 最大允许距离
                for (CloneBall* ball : m_splitBalls) {
                    if (ball && !ball->isRemoved()) {
                        QPointF ballPos = ball->pos();
                        float distanceToCenter = QLineF(ballPos, centerOfMass).length();
                        
                        if (distanceToCenter > maxDistance) {
                            // 太远的球向质心聚拢
                            QPointF direction = centerOfMass - ballPos;
                            float length = QLineF(QPointF(0,0), direction).length();
                            if (length > 0) {
                                direction /= length; // 归一化
                                
                                AIAction gatherAction;
                                gatherAction.dx = direction.x();
                                gatherAction.dy = direction.y();
                                gatherAction.type = ActionType::MOVE;
                                
                                executeActionForBall(ball, gatherAction);
                                continue; // 跳过正常决策
                            }
                        }
                    }
                }
            }
        }
        
        // 为每个分裂球做正常决策
        for (CloneBall* ball : m_splitBalls) {
            if (ball && !ball->isRemoved()) {
                AIAction action;
                
                // 临时设置当前控制的球
                CloneBall* originalPlayerBall = m_playerBall;
                m_playerBall = ball;
                
                // 根据策略选择决策方法
                switch (m_strategy) {
                    case AIStrategy::RANDOM:
                        action = makeRandomDecision();
                        break;
                    case AIStrategy::FOOD_HUNTER:
                        action = makeFoodHunterDecision();
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
        }
        
        // 发送第一个球的动作信号（用于UI显示）
        if (!m_splitBalls.isEmpty() && m_splitBalls.first()) {
            AIAction action = makeFoodHunterDecision(); // 重新计算一次用于显示
            emit actionExecuted(action);
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
        
        // 如果威胁极高且可以分裂，分裂逃跑
        if (totalThreatLevel > 5.0f && m_playerBall->canSplit() && playerScore > 30.0f) {
            return AIAction(escapeDirection.x(), escapeDirection.y(), ActionType::SPLIT);
        }
        
        return AIAction(escapeDirection.x(), escapeDirection.y(), ActionType::MOVE);
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
                        return AIAction(direction.x() / length, direction.y() / length, ActionType::MOVE);
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
                        return AIAction(finalDirection.x(), finalDirection.y(), ActionType::MOVE);
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
                return AIAction(direction.x() / length, direction.y() / length, ActionType::SPLIT);
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
            QPointF direction = bestFood->pos() - playerPos;
            float length = QLineF(QPointF(0,0), direction).length();
            if (length > 0.1f) {
                return AIAction(direction.x() / length, direction.y() / length, ActionType::MOVE);
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
            return AIAction(explorationDirection.x(), explorationDirection.y(), ActionType::MOVE);
        }
    }
    
    // 6. 最后选择：随机探索
    return makeRandomDecision();
}

AIAction SimpleAIPlayer::makeAggressiveDecision() {
    // 攻击策略：寻找较小的玩家进行攻击
    auto nearbyPlayers = getNearbyPlayers(150.0f);
    
    QPointF playerPos = m_playerBall->pos();
    CloneBall* targetPlayer = nullptr;
    float minDistance = std::numeric_limits<float>::max();
    
    for (auto player : nearbyPlayers) {
        if (player != m_playerBall && player->radius() < m_playerBall->radius() * 0.8f) {
            // 这是一个可以攻击的目标
            float distance = QPointF(player->pos() - playerPos).manhattanLength();
            if (distance < minDistance) {
                minDistance = distance;
                targetPlayer = player;
            }
        }
    }
    
    if (targetPlayer) {
        QPointF direction = targetPlayer->pos() - playerPos;
        float length = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());
        
        if (length > 0.1f) {
            float dx = direction.x() / length;
            float dy = direction.y() / length;
            
            // 如果距离合适且可以分裂，执行分裂攻击
            if (length < 50.0f && m_playerBall->canSplit()) {
                return AIAction(dx, dy, ActionType::SPLIT);
            }
            
            return AIAction(dx, dy, ActionType::MOVE);
        }
    }
    
    // 如果没有攻击目标，回到食物猎手模式
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
    
    // 执行移动
    if (action.dx != 0.0f || action.dy != 0.0f) {
        QPointF targetDirection(action.dx, action.dy);
        ball->setTargetDirection(targetDirection);
    }
    
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

// 🔥 新增：多球生存机制的辅助方法
CloneBall* SimpleAIPlayer::getLargestBall() const {
    if (m_splitBalls.isEmpty()) {
        return nullptr;
    }
    
    CloneBall* largest = nullptr;
    float maxScore = 0.0f;
    
    for (CloneBall* ball : m_splitBalls) {
        if (ball && !ball->isRemoved() && ball->score() > maxScore) {
            maxScore = ball->score();
            largest = ball;
        }
    }
    
    return largest;
}

CloneBall* SimpleAIPlayer::getMainControlBall() const {
    // 优先返回当前主球，如果主球无效则返回最大的球
    if (m_playerBall && !m_playerBall->isRemoved()) {
        return m_playerBall;
    }
    
    // 否则返回最大的球
    return getLargestBall();
}

} // namespace AI
} // namespace GoBigger
