#include "ONNXInference.h"
#include <QDebug>
#include <QFileInfo>
#include <stdexcept>
#include <numeric>

// ONNX Runtime包含 - 仅在可用时编译
#ifdef HAS_ONNXRUNTIME
#ifdef _WIN32
#include <onnxruntime_cxx_api.h>
#else
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#endif
#endif // HAS_ONNXRUNTIME

namespace GoBigger {
namespace AI {

ONNXInference::ONNXInference() 
    : m_loaded(false) {
#ifdef HAS_ONNXRUNTIME
    try {
        // 初始化ONNX Runtime环境
        m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "GoBiggerAI");
        m_memoryInfo = std::make_unique<Ort::MemoryInfo>(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU));
        qDebug() << "ONNXInference initialized";
    } catch (const std::exception& e) {
        qWarning() << "Failed to initialize ONNX Runtime:" << e.what();
    }
#else
    qWarning() << "ONNX Runtime not available, model inference disabled";
#endif
}

ONNXInference::~ONNXInference() {
#ifdef HAS_ONNXRUNTIME
    m_session.reset();
    m_memoryInfo.reset();
    m_env.reset();
#endif
    qDebug() << "ONNXInference destroyed";
}

bool ONNXInference::loadModel(const std::string& modelPath) {
#ifndef HAS_ONNXRUNTIME
    qWarning() << "ONNX Runtime not available, cannot load model";
    return false;
#else
    try {
        QFileInfo fileInfo(QString::fromStdString(modelPath));
        if (!fileInfo.exists()) {
            qWarning() << "Model file does not exist:" << QString::fromStdString(modelPath);
            return false;
        }
        
        qDebug() << "Loading ONNX model from:" << QString::fromStdString(modelPath);
        qDebug() << "File size:" << fileInfo.size() << "bytes";
        
        // 创建会话选项
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(1);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC); // 使用基础优化，避免兼容性问题
        
        // 启用详细日志
        sessionOptions.SetLogSeverityLevel(0);
        
        // 创建会话
#ifdef _WIN32
        // Windows使用宽字符路径
        std::wstring wideModelPath;
        wideModelPath.assign(modelPath.begin(), modelPath.end());
        m_session = std::make_unique<Ort::Session>(*m_env, wideModelPath.c_str(), sessionOptions);
#else
        m_session = std::make_unique<Ort::Session>(*m_env, modelPath.c_str(), sessionOptions);
#endif
        
        qDebug() << "ONNX session created successfully";
        
        // 获取输入信息
        size_t numInputNodes = m_session->GetInputCount();
        qDebug() << "Number of input nodes:" << numInputNodes;
        
        if (numInputNodes == 0) {
            qWarning() << "Model has no input nodes!";
            return false;
        }
        
        // 获取输入名称和形状
        Ort::AllocatorWithDefaultOptions allocator;
        m_inputNames.clear();
        m_inputNamesPtrs.clear();
        
        for (size_t i = 0; i < numInputNodes; ++i) {
            auto inputName = m_session->GetInputNameAllocated(i, allocator);
            std::string nameStr(inputName.get());
            m_inputNames.push_back(nameStr);
            qDebug() << "Input" << i << "name:" << nameStr.c_str();
            
            auto inputTypeInfo = m_session->GetInputTypeInfo(i);
            auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
            auto inputShape = inputTensorInfo.GetShape();
            
            qDebug() << "Input" << i << "shape:";
            for (size_t j = 0; j < inputShape.size(); ++j) {
                qDebug() << "  Dim" << j << ":" << inputShape[j];
            }
            
            if (i == 0) {
                m_inputShape = inputShape;
            }
        }
        
        // 获取输出信息
        size_t numOutputNodes = m_session->GetOutputCount();
        qDebug() << "Number of output nodes:" << numOutputNodes;
        
        if (numOutputNodes == 0) {
            qWarning() << "Model has no output nodes!";
            return false;
        }

        m_outputNames.clear();
        m_outputNamesPtrs.clear();
        
        for (size_t i = 0; i < numOutputNodes; ++i) {
            auto outputName = m_session->GetOutputNameAllocated(i, allocator);
            std::string nameStr(outputName.get());
            m_outputNames.push_back(nameStr);
            qDebug() << "Output" << i << "name:" << nameStr.c_str();
            
            auto outputTypeInfo = m_session->GetOutputTypeInfo(i);
            auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
            auto outputShape = outputTensorInfo.GetShape();
            
            qDebug() << "Output" << i << "shape:";
            for (size_t j = 0; j < outputShape.size(); ++j) {
                qDebug() << "  Dim" << j << ":" << outputShape[j];
            }
            
            if (i == 0) {
                m_outputShape = outputShape;
            }
        }
        
        // 验证基本兼容性
        if (numInputNodes > 1) {
            qWarning() << "Warning: Model has multiple inputs, will only use the first one";
        }
          if (numOutputNodes > 1) {
            qWarning() << "Warning: Model has multiple outputs, will only use the first one";
        }
        
        // 创建指向字符串数据的指针数组
        m_inputNamesPtrs.clear();
        for (const auto& name : m_inputNames) {
            m_inputNamesPtrs.push_back(name.c_str());
        }
        
        m_outputNamesPtrs.clear();
        for (const auto& name : m_outputNames) {
            m_outputNamesPtrs.push_back(name.c_str());
        }

        m_loaded = true;
        
        qDebug() << "ONNX model loaded successfully";
        qDebug() << "Primary input shape dimensions:" << m_inputShape.size();
        qDebug() << "Primary output shape dimensions:" << m_outputShape.size();
        
        return true;
        
    } catch (const Ort::Exception& e) {
        qWarning() << "ONNX Runtime exception:" << e.what();
        qWarning() << "Error code:" << e.GetOrtErrorCode();
        m_loaded = false;
        return false;
    } catch (const std::exception& e) {
        qWarning() << "Failed to load ONNX model:" << e.what();
        m_loaded = false;
        return false;
    }
#endif // HAS_ONNXRUNTIME
}

std::vector<float> ONNXInference::predict(const std::vector<float>& observation) {
#ifndef HAS_ONNXRUNTIME
    qWarning() << "ONNX Runtime not available, cannot predict";
    return {};
#else
    if (!m_loaded || !m_session) {
        qWarning() << "Model not loaded, cannot predict";
        return {};
    }
    
    try {
        // 获取输入尺寸（支持动态输入大小）
        size_t expectedInputSize = getInputSize();
        
        // 如果期望大小为0或者观察大小不匹配，尝试适应
        if (expectedInputSize == 0 || observation.size() != expectedInputSize) {
            qDebug() << "Input size flexible mode - Expected:" << expectedInputSize 
                     << "Got:" << observation.size();
            
            // 尝试使用实际观察大小
            expectedInputSize = observation.size();
        }
        
        if (observation.empty()) {
            qWarning() << "Empty observation provided";
            return {};
        }
        
        // 准备输入张量形状
        std::vector<int64_t> inputShapeForBatch = m_inputShape;
        
        // 处理动态批处理大小
        if (!inputShapeForBatch.empty()) {
            inputShapeForBatch[0] = 1; // batch size = 1
            
            // 如果有第二个维度且为-1（动态），设置为实际观察大小
            if (inputShapeForBatch.size() > 1 && inputShapeForBatch[1] == -1) {
                inputShapeForBatch[1] = static_cast<int64_t>(observation.size());
            }
        } else {
            // 如果没有形状信息，创建一个默认的
            inputShapeForBatch = {1, static_cast<int64_t>(observation.size())};
        }
        
        qDebug() << "Using input tensor shape:";
        for (size_t i = 0; i < inputShapeForBatch.size(); ++i) {
            qDebug() << "  Dim" << i << ":" << inputShapeForBatch[i];
        }
        
        // 创建输入张量
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            *m_memoryInfo, 
            const_cast<float*>(observation.data()), 
            observation.size(),
            inputShapeForBatch.data(), 
            inputShapeForBatch.size()
        );
        
        // 执行推理
        auto outputTensors = m_session->Run(
            Ort::RunOptions{nullptr},
            m_inputNamesPtrs.data(),
            &inputTensor,
            1,
            m_outputNamesPtrs.data(),
            std::min(static_cast<size_t>(1), m_outputNamesPtrs.size()) // 只使用第一个输出
        );
        
        // 获取输出数据
        if (outputTensors.empty()) {
            qWarning() << "No output from model";
            return {};
        }
        
        auto& outputTensor = outputTensors[0];
        float* outputData = outputTensor.GetTensorMutableData<float>();
        auto outputShape = outputTensor.GetTensorTypeAndShapeInfo().GetShape();
        
        qDebug() << "Output tensor shape:";
        for (size_t i = 0; i < outputShape.size(); ++i) {
            qDebug() << "  Dim" << i << ":" << outputShape[i];
        }
        
        size_t outputSize = std::accumulate(outputShape.begin(), outputShape.end(), 1, std::multiplies<size_t>());
        
        std::vector<float> result(outputData, outputData + outputSize);
        
        qDebug() << "ONNX prediction completed, output size:" << result.size();
        if (!result.empty()) {
            qDebug() << "First few outputs:";
            for (size_t i = 0; i < std::min(result.size(), static_cast<size_t>(5)); ++i) {
                qDebug() << "  Output[" << i << "]:" << result[i];
            }
        }
        
        return result;
        
    } catch (const Ort::Exception& e) {
        qWarning() << "ONNX Runtime prediction exception:" << e.what();
        qWarning() << "Error code:" << e.GetOrtErrorCode();
        return {};
    } catch (const std::exception& e) {
        qWarning() << "ONNX prediction failed:" << e.what();
        return {};
    }
#endif // HAS_ONNXRUNTIME
}

size_t ONNXInference::getInputSize() const {
#ifndef HAS_ONNXRUNTIME
    return 400; // 默认观察空间大小
#else
    if (m_inputShape.empty() || m_inputShape.size() < 2) {
        return 0;
    }
    // 跳过batch维度（第一个维度），计算特征数量
    size_t size = 1;
    for (size_t i = 1; i < m_inputShape.size(); ++i) {
        if (m_inputShape[i] > 0) {
            size *= m_inputShape[i];
        }
    }
    return size;
#endif
}

size_t ONNXInference::getOutputSize() const {
#ifndef HAS_ONNXRUNTIME
    return 3; // 默认动作空间大小
#else
    if (m_outputShape.empty() || m_outputShape.size() < 2) {
        return 0;
    }
    // 跳过batch维度（第一个维度），计算输出数量
    size_t size = 1;
    for (size_t i = 1; i < m_outputShape.size(); ++i) {
        if (m_outputShape[i] > 0) {
            size *= m_outputShape[i];
        }
    }
    return size;
#endif
}

} // namespace AI
} // namespace GoBigger
