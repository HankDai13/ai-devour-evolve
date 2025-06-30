#pragma once
#include <vector>
#include <string>
#include <memory>

namespace GoBigger {
namespace AI {

// AI模型推理接口
class ModelInference {
public:
    virtual ~ModelInference() = default;
    
    // 加载模型
    virtual bool loadModel(const std::string& model_path) = 0;
    
    // 执行推理
    virtual std::vector<float> predict(const std::vector<float>& observation) = 0;
    
    // 获取动作
    virtual std::vector<float> getAction(const std::vector<float>& observation) = 0;
};

// ONNX模型推理实现
class ONNXInference : public ModelInference {
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
public:
    ONNXInference();
    ~ONNXInference();
    
    bool loadModel(const std::string& model_path) override;
    std::vector<float> predict(const std::vector<float>& observation) override;
    std::vector<float> getAction(const std::vector<float>& observation) override;
};

// 常量定义
constexpr int OBSERVATION_SIZE = 400;
constexpr int ACTION_SIZE = 3;

// 动作类型
enum class ActionType {
    MOVE = 0,     // 移动
    SPLIT = 1,    // 分裂
    EJECT = 2     // 喷射
};

// 动作结构
struct Action {
    float dx;        // x方向移动 [-1.0, 1.0]
    float dy;        // y方向移动 [-1.0, 1.0]
    ActionType type; // 动作类型
    
    Action(float dx = 0.0f, float dy = 0.0f, ActionType type = ActionType::MOVE)
        : dx(dx), dy(dy), type(type) {}
};

} // namespace AI
} // namespace GoBigger
