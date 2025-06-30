#pragma once
#include <vector>
#include <string>
#include <memory>

#ifdef HAS_ONNXRUNTIME
// ONNX Runtime前向声明 - 仅在可用时声明
namespace Ort {
    class Session;
    class Env;
    struct Value;
    class MemoryInfo;
}
#endif

namespace GoBigger {
namespace AI {

// ONNX Runtime推理类
class ONNXInference {
private:
#ifdef HAS_ONNXRUNTIME
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;
    std::unique_ptr<Ort::MemoryInfo> m_memoryInfo;
    
    // 模型输入输出信息
    std::vector<std::string> m_inputNames;
    std::vector<std::string> m_outputNames;
    std::vector<const char*> m_inputNamesPtrs;    // 指向字符串数据的指针
    std::vector<const char*> m_outputNamesPtrs;   // 指向字符串数据的指针
    std::vector<int64_t> m_inputShape;
    std::vector<int64_t> m_outputShape;
#endif
    
    bool m_loaded;
    
public:
    ONNXInference();
    ~ONNXInference();
    
    // 加载ONNX模型
    bool loadModel(const std::string& modelPath);
    
    // 执行推理，返回动作概率分布
    std::vector<float> predict(const std::vector<float>& observation);
    
    // 检查模型是否已加载
    bool isLoaded() const { return m_loaded; }
    
    // 获取输入/输出维度信息
    size_t getInputSize() const;
    size_t getOutputSize() const;
};

} // namespace AI
} // namespace GoBigger
