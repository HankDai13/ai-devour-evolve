#include "AIPlayer.h"
#include "CloneBall.h"
#include "GameManager.h"
#include "BaseBall.h"
#include "FoodBall.h"
#include "ThornsBall.h"
#include "SporeBall.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QFileInfo>
#include <torch/torch.h>
#include <torch/script.h>
#include <algorithm>
#include <cmath>

namespace GoBigger {
namespace AI {

// TorchInference 实现
TorchInference::TorchInference() : m_loaded(false) {
}

TorchInference::~TorchInference() {
}

bool TorchInference::loadModel(const QString& model_path) {
    try {
        // 检查文件是否存在
        QFileInfo fileInfo(model_path);
        if (!fileInfo.exists()) {
            qWarning() << "AI model file does not exist:" << model_path;
            return false;
        }
        
        // 加载TorchScript模型
        m_model = std::make_unique<torch::jit::Module>(torch::jit::load(model_path.toStdString()));
        m_model->eval(); // 设置为评估模式
        
        m_loaded = true;
        qDebug() << "Successfully loaded AI model from:" << model_path;
        return true;
        
    } catch (const std::exception& e) {
        qWarning() << "Failed to load AI model:" << e.what();
        m_loaded = false;
        return false;
    }
}

AIAction TorchInference::predict(const std::vector<float>& observation) {
    if (!m_loaded || !m_model) {
        qWarning() << "AI model not loaded";
        return AIAction(); // 返回默认动作
    }
    
    try {
        // 创建输入tensor
        torch::Tensor input_tensor = torch::from_blob(
            const_cast<float*>(observation.data()), 
            {1, static_cast<long>(observation.size())}, 
            torch::kFloat
        );
        
        // 执行推理
        std::vector<torch::jit::IValue> inputs;
        inputs.push_back(input_tensor);
        
        at::Tensor output = m_model->forward(inputs).toTensor();
        
        // 提取动作值
        auto output_accessor = output.accessor<float, 2>();
        float dx = std::clamp(output_accessor[0][0], -1.0f, 1.0f);
        float dy = std::clamp(output_accessor[0][1], -1.0f, 1.0f);
        float action_type_raw = output_accessor[0][2];
        
        // 将动作类型转换为离散值
        ActionType action_type = ActionType::MOVE;
        if (action_type_raw >= 1.5f) {
            action_type = ActionType::EJECT;
        } else if (action_type_raw >= 0.5f) {
            action_type = ActionType::SPLIT;
        }
        
        return AIAction(dx, dy, action_type);
        
    } catch (const std::exception& e) {
        qWarning() << "AI inference error:" << e.what();
        return AIAction(); // 返回默认动作
    }
}

// AIPlayer 实现
AIPlayer::AIPlayer(CloneBall* playerBall, QObject* parent)
    : QObject(parent)
    , m_playerBall(playerBall)
    , m_inference(std::make_unique<TorchInference>())
    , m_decisionTimer(new QTimer(this))
    , m_aiActive(false)
    , m_decisionInterval(100) // 默认100ms决策间隔
{
    // 连接定时器
    connect(m_decisionTimer, &QTimer::timeout, this, &AIPlayer::makeDecision);
    
    // 监听玩家球被销毁的信号
    if (m_playerBall) {
        connect(m_playerBall, &QObject::destroyed, this, &AIPlayer::onPlayerBallDestroyed);
    }
    
    qDebug() << "AIPlayer created for ball:" << (m_playerBall ? m_playerBall->ballId() : -1);
}

AIPlayer::~AIPlayer() {
    stopAI();
    qDebug() << "AIPlayer destroyed";
}

bool AIPlayer::loadAIModel(const QString& model_path) {
    if (!m_inference) {
        qWarning() << "AI inference engine not initialized";
        return false;
    }
    
    bool success = m_inference->loadModel(model_path);
    if (success) {
        qDebug() << "AI model loaded successfully for player ball:" 
                 << (m_playerBall ? m_playerBall->ballId() : -1);
    } else {
        qWarning() << "Failed to load AI model for player ball:" 
                   << (m_playerBall ? m_playerBall->ballId() : -1);
    }
    
    return success;
}

void AIPlayer::startAI() {
    if (!m_playerBall) {
        qWarning() << "Cannot start AI: no player ball";
        return;
    }
    
    if (!m_inference || !m_inference->isLoaded()) {
        qWarning() << "Cannot start AI: model not loaded";
        return;
    }
    
    if (m_aiActive) {
        qDebug() << "AI already active";
        return;
    }
    
    m_aiActive = true;
    m_decisionTimer->start(m_decisionInterval);
    qDebug() << "AI started for player ball:" << m_playerBall->ballId() 
             << "with decision interval:" << m_decisionInterval << "ms";
}

void AIPlayer::stopAI() {
    if (!m_aiActive) {
        return;
    }
    
    m_aiActive = false;
    m_decisionTimer->stop();
    qDebug() << "AI stopped for player ball:" << (m_playerBall ? m_playerBall->ballId() : -1);
}

void AIPlayer::setDecisionInterval(int interval_ms) {
    m_decisionInterval = std::max(50, interval_ms); // 最小50ms
    if (m_aiActive) {
        m_decisionTimer->start(m_decisionInterval);
    }
}

void AIPlayer::makeDecision() {
    if (!m_aiActive || !m_playerBall || !m_inference || !m_inference->isLoaded()) {
        return;
    }
    
    try {
        // 生成观测数据
        std::vector<float> observation = generateObservation();
        emit observationGenerated(observation);
        
        // 执行AI推理
        AIAction action = m_inference->predict(observation);
        
        // 执行动作
        executeAction(action);
        emit actionExecuted(action);
        
    } catch (const std::exception& e) {
        QString error = QString("AI decision error: %1").arg(e.what());
        qWarning() << error;
        emit inferenceError(error);
    }
}

void AIPlayer::onPlayerBallDestroyed() {
    qDebug() << "Player ball destroyed, stopping AI";
    m_playerBall = nullptr;
    stopAI();
}

std::vector<float> AIPlayer::generateObservation() {
    // 创建400维观测向量（与训练模型一致）
    std::vector<float> observation(400, 0.0f);
    
    if (!m_playerBall || !m_playerBall->scene()) {
        return observation;
    }
    
    int idx = 0;
    
    // 1. 提取玩家特征 (约50维)
    extractPlayerFeatures(observation);
    
    // 2. 提取环境特征 (约50维)
    extractEnvironmentFeatures(observation);
    
    // 3. 提取附近球体特征 (约300维)
    extractNearbyBallsFeatures(observation);
    
    return observation;
}

void AIPlayer::extractPlayerFeatures(std::vector<float>& observation) {
    if (!m_playerBall) return;
    
    int idx = 0;
    
    // 玩家位置 (归一化到[-1,1])
    QPointF pos = m_playerBall->pos();
    observation[idx++] = pos.x() / 400.0f; // 假设游戏区域是800x800
    observation[idx++] = pos.y() / 400.0f;
    
    // 玩家大小/分数
    observation[idx++] = m_playerBall->getScore() / 1000.0f; // 归一化分数
    observation[idx++] = m_playerBall->radius() / 50.0f;     // 归一化半径
    
    // 玩家速度
    QPointF velocity = m_playerBall->getVelocity();
    observation[idx++] = velocity.x() / 10.0f;
    observation[idx++] = velocity.y() / 10.0f;
    
    // 玩家能力状态
    observation[idx++] = m_playerBall->canSplit() ? 1.0f : 0.0f;
    observation[idx++] = m_playerBall->canEject() ? 1.0f : 0.0f;
    
    // 填充到50维
    while (idx < 50) {
        observation[idx++] = 0.0f;
    }
}

void AIPlayer::extractEnvironmentFeatures(std::vector<float>& observation) {
    if (!m_playerBall || !m_playerBall->scene()) return;
    
    int idx = 50; // 从第50维开始
    
    // 游戏边界距离
    QPointF pos = m_playerBall->pos();
    observation[idx++] = (400.0f - pos.x()) / 400.0f;  // 右边界距离
    observation[idx++] = (pos.x() + 400.0f) / 400.0f;  // 左边界距离
    observation[idx++] = (400.0f - pos.y()) / 400.0f;  // 下边界距离
    observation[idx++] = (pos.y() + 400.0f) / 400.0f;  // 上边界距离
    
    // 获取场景中所有球体的统计信息
    QGraphicsScene* scene = m_playerBall->scene();
    auto items = scene->items();
    
    int foodCount = 0, thornsCount = 0, playerCount = 0, sporeCount = 0;
    for (auto item : items) {
        if (dynamic_cast<FoodBall*>(item)) foodCount++;
        else if (dynamic_cast<ThornsBall*>(item)) thornsCount++;
        else if (dynamic_cast<CloneBall*>(item)) playerCount++;
        else if (dynamic_cast<SporeBall*>(item)) sporeCount++;
    }
    
    observation[idx++] = foodCount / 1000.0f;    // 归一化食物数量
    observation[idx++] = thornsCount / 20.0f;    // 归一化荆棘数量
    observation[idx++] = playerCount / 10.0f;    // 归一化玩家数量
    observation[idx++] = sporeCount / 100.0f;    // 归一化孢子数量
    
    // 填充到100维
    while (idx < 100) {
        observation[idx++] = 0.0f;
    }
}

void AIPlayer::extractNearbyBallsFeatures(std::vector<float>& observation) {
    if (!m_playerBall || !m_playerBall->scene()) return;
    
    int idx = 100; // 从第100维开始
    const int MAX_NEARBY_BALLS = 100; // 最多考虑100个附近的球
    const float VIEW_RADIUS = 200.0f; // 视野半径
    
    QPointF playerPos = m_playerBall->pos();
    QGraphicsScene* scene = m_playerBall->scene();
    
    // 获取视野范围内的所有球体
    QRectF viewRect(playerPos.x() - VIEW_RADIUS, playerPos.y() - VIEW_RADIUS,
                    2 * VIEW_RADIUS, 2 * VIEW_RADIUS);
    auto nearbyItems = scene->items(viewRect);
    
    // 按距离排序
    std::vector<std::pair<float, BaseBall*>> sortedBalls;
    
    for (auto item : nearbyItems) {
        BaseBall* ball = dynamic_cast<BaseBall*>(item);
        if (ball && ball != m_playerBall) {
            QPointF ballPos = ball->pos();
            float distance = QPointF(ballPos - playerPos).manhattanLength();
            if (distance <= VIEW_RADIUS) {
                sortedBalls.push_back({distance, ball});
            }
        }
    }
    
    // 按距离排序
    std::sort(sortedBalls.begin(), sortedBalls.end());
    
    // 提取最近的球体特征
    int ballCount = 0;
    for (const auto& pair : sortedBalls) {
        if (ballCount >= MAX_NEARBY_BALLS) break;
        
        BaseBall* ball = pair.second;
        QPointF ballPos = ball->pos();
        QPointF relativePos = ballPos - playerPos;
        
        // 每个球3个特征: 相对x, 相对y, 类型/大小
        observation[idx++] = relativePos.x() / VIEW_RADIUS; // 归一化相对位置
        observation[idx++] = relativePos.y() / VIEW_RADIUS;
        
        // 球的类型和大小编码
        if (dynamic_cast<FoodBall*>(ball)) {
            observation[idx++] = 0.1f; // 食物标记
        } else if (dynamic_cast<ThornsBall*>(ball)) {
            observation[idx++] = -0.5f; // 荆棘标记（负值表示危险）
        } else if (dynamic_cast<CloneBall*>(ball)) {
            CloneBall* cloneBall = static_cast<CloneBall*>(ball);
            float sizeRatio = cloneBall->radius() / m_playerBall->radius();
            observation[idx++] = sizeRatio > 1.2f ? -1.0f : (sizeRatio < 0.8f ? 1.0f : 0.0f);
        } else if (dynamic_cast<SporeBall*>(ball)) {
            observation[idx++] = 0.3f; // 孢子标记
        } else {
            observation[idx++] = 0.0f;
        }
        
        ballCount++;
    }
    
    // 填充剩余维度到400
    while (idx < 400) {
        observation[idx++] = 0.0f;
    }
}

void AIPlayer::executeAction(const AIAction& action) {
    if (!m_playerBall) return;
    
    // 执行移动
    if (action.dx != 0.0f || action.dy != 0.0f) {
        QPointF currentPos = m_playerBall->pos();
        QPointF targetDirection(action.dx, action.dy);
        
        // 设置目标方向（CloneBall会处理实际的移动）
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

} // namespace AI
} // namespace GoBigger
