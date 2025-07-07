#pragma once
#include <QObject>
#include <QTimer>
#include <QPointF>
#include <memory>
#include <vector>
#include "CloneBall.h"

// Forward declarations
namespace torch { namespace jit { struct Module; } }

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

// AI模型推理接口
class AIModelInference {
public:
    virtual ~AIModelInference() = default;
    
    // 加载模型
    virtual bool loadModel(const QString& model_path) = 0;
    
    // 执行推理，返回动作
    virtual AIAction predict(const std::vector<float>& observation) = 0;
    
    // 检查模型是否已加载
    virtual bool isLoaded() const = 0;
};

// LibTorch模型推理实现
class TorchInference : public AIModelInference {
private:
    std::unique_ptr<torch::jit::Module> m_model;
    bool m_loaded;
    
public:
    TorchInference();
    ~TorchInference();
    
    bool loadModel(const QString& model_path) override;
    AIAction predict(const std::vector<float>& observation) override;
    bool isLoaded() const override { return m_loaded; }
};

// AI玩家类
class AIPlayer : public QObject {
    Q_OBJECT
    
public:
    AIPlayer(CloneBall* playerBall, QObject* parent = nullptr);
    ~AIPlayer();
    
    // 设置AI模型
    bool loadAIModel(const QString& model_path);
    
    // 启动/停止AI控制
    void startAI();
    void stopAI();
    bool isAIActive() const { return m_aiActive; }
    
    // 获取关联的玩家球
    CloneBall* getPlayerBall() const { return m_playerBall; }
    
    // 设置决策间隔（毫秒）
    void setDecisionInterval(int interval_ms);
    int getDecisionInterval() const { return m_decisionInterval; }

signals:
    void actionExecuted(const AIAction& action);
    void observationGenerated(const std::vector<float>& observation);
    void inferenceError(const QString& error);

private slots:
    void makeDecision();
    void onPlayerBallDestroyed();

private:
    CloneBall* m_playerBall;
    std::unique_ptr<AIModelInference> m_inference;
    QTimer* m_decisionTimer;
    bool m_aiActive;
    int m_decisionInterval; // 决策间隔（毫秒）
    
    // 生成观测数据
    std::vector<float> generateObservation();
    
    // 执行AI动作
    void executeAction(const AIAction& action);
    
    // 观测特征提取
    void extractPlayerFeatures(std::vector<float>& observation);
    void extractEnvironmentFeatures(std::vector<float>& observation);
    void extractNearbyBallsFeatures(std::vector<float>& observation);
};

} // namespace AI
} // namespace GoBigger
