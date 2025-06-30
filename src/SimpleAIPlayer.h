#pragma once
#include <QObject>
#include <QTimer>
#include <QPointF>
#include <memory>
#include <vector>
#include <string>
#include "CloneBall.h"
#include "ONNXInference.h"

// Forward declarations
class BaseBall;
class FoodBall;

namespace GoBigger {
namespace AI {

// AI动作类型
enum class ActionType {
    MOVE = 0,     // 移动
    SPLIT = 1,    // 分裂
    EJECT = 2     // 喷射
};

// AI动作结构
struct AIAction {
    float dx;        // x方向移动 [-1.0, 1.0]
    float dy;        // y方向移动 [-1.0, 1.0]
    ActionType type; // 动作类型
    
    AIAction(float dx = 0.0f, float dy = 0.0f, ActionType type = ActionType::MOVE)
        : dx(dx), dy(dy), type(type) {}
};

// AI模型推理接口（简化版本，兼容性更好）
class SimpleModelInference {
public:
    virtual ~SimpleModelInference() = default;
    
    // 加载模型
    virtual bool loadModel(const QString& model_path) = 0;
    
    // 执行推理，返回动作
    virtual AIAction predict(const std::vector<float>& observation) = 0;
    
    // 检查模型是否已加载
    virtual bool isLoaded() const = 0;
};

// 简化的AI玩家类（不依赖复杂的推理）
class SimpleAIPlayer : public QObject {
    Q_OBJECT
    
public:
    SimpleAIPlayer(CloneBall* playerBall, QObject* parent = nullptr);
    ~SimpleAIPlayer();
    
    // 启动/停止AI控制
    void startAI();
    void stopAI();
    bool isAIActive() const { return m_aiActive; }
    
    // 获取关联的玩家球
    CloneBall* getPlayerBall() const { return m_playerBall; }
    
    // 设置决策间隔（毫秒）
    void setDecisionInterval(int interval_ms);
    int getDecisionInterval() const { return m_decisionInterval; }
    
    // 设置AI策略类型
    enum class AIStrategy {
        RANDOM,      // 随机移动
        FOOD_HUNTER, // 寻找食物
        AGGRESSIVE,  // 攻击性策略
        MODEL_BASED  // 基于模型的策略（将来实现）
    };
    
    void setAIStrategy(AIStrategy strategy) { m_strategy = strategy; }
    AIStrategy getAIStrategy() const { return m_strategy; }
    
    // 模型推理相关
    bool loadAIModel(const QString& modelPath);
    bool isModelLoaded() const;
    void setObservationSize(int size) { m_observationSize = size; }

signals:
    void actionExecuted(const AIAction& action);
    void strategyChanged(AIStrategy newStrategy);
    void aiPlayerDestroyed(GoBigger::AI::SimpleAIPlayer* aiPlayer); // 新增：AI被销毁信号

private slots:
    void makeDecision();
    void onPlayerBallDestroyed();
    void onPlayerBallRemoved();  // 新增：处理球被移除
    void onSplitPerformed(CloneBall* originalBall, const QVector<CloneBall*>& newBalls);
    void onBallDestroyed(QObject* ball);

private:
    CloneBall* m_playerBall;
    QVector<CloneBall*> m_splitBalls; // 管理分裂后的所有球体
    QTimer* m_decisionTimer;
    bool m_aiActive;
    int m_decisionInterval; // 决策间隔（毫秒）
    AIStrategy m_strategy;
    
    // 模型推理相关
    std::unique_ptr<ONNXInference> m_onnxInference;
    int m_observationSize; // 观察向量大小（默认400）
    
    // 不同策略的实现
    AIAction makeRandomDecision();
    AIAction makeFoodHunterDecision();
    AIAction makeAggressiveDecision();
    AIAction makeModelBasedDecision();
    
    // 特征提取
    std::vector<float> extractObservation();
    
    // 执行AI动作
    void executeAction(const AIAction& action);
    void executeActionForBall(CloneBall* ball, const AIAction& action);

    // 获取附近的球体信息
    std::vector<BaseBall*> getNearbyBalls(float radius = 100.0f);
    std::vector<FoodBall*> getNearbyFood(float radius = 150.0f);
    std::vector<CloneBall*> getNearbyPlayers(float radius = 120.0f);
};

} // namespace AI
} // namespace GoBigger
