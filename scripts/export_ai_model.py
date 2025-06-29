#!/usr/bin/env python3
"""
AI模型导出工具
将训练好的PPO模型导出为C++可用的格式
"""
import os
import sys
import torch
import numpy as np
from pathlib import Path

# 添加项目路径
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

from gobigger_gym_env import GoBiggerEnv

try:
    from stable_baselines3 import PPO
    import onnx
    print("✅ 成功导入所需库")
except ImportError as e:
    print(f"❌ 导入失败: {e}")
    print("请安装: pip install stable-baselines3 onnx")
    sys.exit(1)

class ModelExporter:
    """AI模型导出工具类"""
    
    def __init__(self):
        self.observation_space_size = 400  # GoBigger环境的观察空间大小
        
    def find_best_model(self, search_dirs=["models", "checkpoints"]):
        """查找最佳可用模型"""
        model_files = []
        
        for search_dir in search_dirs:
            if not os.path.exists(search_dir):
                continue
                
            for file in os.listdir(search_dir):
                if file.endswith('.zip') and 'PPO' in file:
                    model_path = os.path.join(search_dir, file)
                    model_files.append({
                        'path': model_path,
                        'name': file,
                        'size': os.path.getsize(model_path),
                        'modified': os.path.getmtime(model_path)
                    })
        
        if not model_files:
            return None
            
        # 按修改时间排序，取最新的
        model_files.sort(key=lambda x: x['modified'], reverse=True)
        return model_files[0]['path']
    
    def load_model(self, model_path=None):
        """加载PPO模型"""
        if model_path is None:
            model_path = self.find_best_model()
            
        if model_path is None or not os.path.exists(model_path):
            print("❌ 未找到可用的模型文件")
            print("请先运行训练脚本生成模型")
            return None
            
        print(f"📦 加载模型: {model_path}")
        try:
            model = PPO.load(model_path)
            print(f"✅ 成功加载模型: {os.path.basename(model_path)}")
            return model
        except Exception as e:
            print(f"❌ 加载模型失败: {e}")
            return None
    
    def create_dummy_input(self):
        """创建用于导出的虚拟输入"""
        # 创建符合GoBigger观察空间的虚拟输入
        return torch.randn(1, self.observation_space_size).float()
    
    def export_to_onnx(self, model, output_path="exported_models/ai_model.onnx"):
        """导出模型为ONNX格式"""
        print("🔄 开始导出ONNX模型...")
        
        # 确保输出目录存在
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        
        try:
            # 创建虚拟输入
            dummy_input = self.create_dummy_input()
            
            # 🔥 确保模型和输入在同一设备上（CPU）
            device = torch.device('cpu')
            model.policy = model.policy.to(device)
            dummy_input = dummy_input.to(device)
            
            # 设置模型为评估模式
            model.policy.eval()
            
            # 导出ONNX
            torch.onnx.export(
                model.policy,  # 模型
                dummy_input,   # 虚拟输入
                output_path,   # 输出路径
                export_params=True,        # 导出参数
                opset_version=11,          # ONNX操作集版本
                do_constant_folding=True,  # 常量折叠优化
                input_names=['observation'],  # 输入名称
                output_names=['action_logits', 'value'],  # 输出名称
                dynamic_axes={
                    'observation': {0: 'batch_size'},
                    'action_logits': {0: 'batch_size'},
                    'value': {0: 'batch_size'}
                }
            )
            
            print(f"✅ ONNX模型导出成功: {output_path}")
            
            # 验证ONNX模型
            self.verify_onnx_model(output_path, dummy_input, model)
            
        except Exception as e:
            print(f"❌ ONNX导出失败: {e}")
            return False
        
        return True
    
    def export_to_torchscript(self, model, output_path="exported_models/ai_model_traced.pt"):
        """导出模型为TorchScript格式"""
        print("🔄 开始导出TorchScript模型...")
        
        # 确保输出目录存在
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        
        try:
            # 创建虚拟输入
            dummy_input = self.create_dummy_input()
            
            # 🔥 确保模型和输入在同一设备上（CPU）
            device = torch.device('cpu')
            model.policy = model.policy.to(device)
            dummy_input = dummy_input.to(device)
            
            # 设置模型为评估模式
            model.policy.eval()
            
            # 使用trace导出
            with torch.no_grad():
                traced_model = torch.jit.trace(model.policy, dummy_input)
            
            # 保存模型
            traced_model.save(output_path)
            
            print(f"✅ TorchScript模型导出成功: {output_path}")
            
            # 验证TorchScript模型
            self.verify_torchscript_model(output_path, dummy_input, model)
            
        except Exception as e:
            print(f"❌ TorchScript导出失败: {e}")
            return False
        
        return True
    
    def verify_onnx_model(self, onnx_path, test_input, original_model):
        """验证ONNX模型输出一致性"""
        try:
            import onnxruntime as ort
            
            # 创建ONNX推理会话
            ort_session = ort.InferenceSession(onnx_path)
            
            # 获取原始模型输出
            with torch.no_grad():
                original_output = original_model.policy(test_input)
                if isinstance(original_output, tuple):
                    original_action_logits = original_output[0].numpy()
                    original_value = original_output[1].numpy()
                else:
                    original_action_logits = original_output.numpy()
                    original_value = None
            
            # 获取ONNX模型输出
            ort_inputs = {ort_session.get_inputs()[0].name: test_input.numpy()}
            ort_outputs = ort_session.run(None, ort_inputs)
            
            # 比较输出
            action_logits_diff = np.abs(original_action_logits - ort_outputs[0]).max()
            print(f"📊 动作logits最大差异: {action_logits_diff:.6f}")
            
            if len(ort_outputs) > 1 and original_value is not None:
                value_diff = np.abs(original_value - ort_outputs[1]).max()
                print(f"📊 价值函数最大差异: {value_diff:.6f}")
            
            if action_logits_diff < 1e-5:
                print("✅ ONNX模型验证通过")
                return True
            else:
                print("⚠️  ONNX模型输出差异较大，请检查")
                return False
                
        except ImportError:
            print("⚠️  未安装onnxruntime，跳过ONNX验证")
            print("安装命令: pip install onnxruntime")
            return True
        except Exception as e:
            print(f"❌ ONNX验证失败: {e}")
            return False
    
    def verify_torchscript_model(self, script_path, test_input, original_model):
        """验证TorchScript模型输出一致性"""
        try:
            # 加载TorchScript模型
            loaded_model = torch.jit.load(script_path)
            
            # 获取原始模型输出
            with torch.no_grad():
                original_output = original_model.policy(test_input)
                if isinstance(original_output, tuple):
                    original_action_logits = original_output[0]
                    original_value = original_output[1]
                else:
                    original_action_logits = original_output
                    original_value = None
            
            # 获取TorchScript模型输出
            with torch.no_grad():
                script_output = loaded_model(test_input)
                if isinstance(script_output, tuple):
                    script_action_logits = script_output[0]
                    script_value = script_output[1]
                else:
                    script_action_logits = script_output
                    script_value = None
            
            # 比较输出
            action_logits_diff = torch.abs(original_action_logits - script_action_logits).max().item()
            print(f"📊 动作logits最大差异: {action_logits_diff:.6f}")
            
            if original_value is not None and script_value is not None:
                value_diff = torch.abs(original_value - script_value).max().item()
                print(f"📊 价值函数最大差异: {value_diff:.6f}")
            
            if action_logits_diff < 1e-5:
                print("✅ TorchScript模型验证通过")
                return True
            else:
                print("⚠️  TorchScript模型输出差异较大，请检查")
                return False
                
        except Exception as e:
            print(f"❌ TorchScript验证失败: {e}")
            return False
    
    def create_model_info(self, model, output_dir="exported_models"):
        """创建模型信息文件"""
        info = {
            "model_type": "PPO",
            "observation_space_size": self.observation_space_size,
            "action_space": {
                "type": "continuous",
                "shape": [3],  # [dx, dy, action_type]
                "low": [-1.0, -1.0, 0],
                "high": [1.0, 1.0, 2]
            },
            "input_format": "float32 array of shape (batch_size, 400)",
            "output_format": {
                "action_logits": "float32 array of shape (batch_size, 3)",
                "value": "float32 array of shape (batch_size, 1)"
            },
            "preprocessing": {
                "normalization": "Environment handles normalization",
                "feature_extraction": "Raw observation from GoBigger environment"
            },
            "usage_notes": [
                "Model expects observations from GoBiggerEnv with 400 features",
                "Action type should be rounded to nearest integer (0, 1, or 2)",
                "dx, dy should be clipped to [-1.0, 1.0] range",
                "Model was trained with Gemini-optimized hyperparameters"
            ]
        }
        
        import json
        info_path = os.path.join(output_dir, "model_info.json")
        with open(info_path, 'w', encoding='utf-8') as f:
            json.dump(info, f, indent=2, ensure_ascii=False)
        
        print(f"📋 模型信息文件已保存: {info_path}")
        return info_path
    
    def create_cpp_header(self, output_dir="exported_models"):
        """创建C++集成所需的头文件"""
        header_content = '''#pragma once
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
'''
        
        header_path = os.path.join(output_dir, "AIModelInterface.h")
        with open(header_path, 'w', encoding='utf-8') as f:
            f.write(header_content)
        
        print(f"📄 C++头文件已生成: {header_path}")
        return header_path

def main():
    """主函数"""
    print("🚀 AI模型导出工具")
    print("=" * 50)
    
    exporter = ModelExporter()
    
    # 1. 查找和加载模型
    model = exporter.load_model()
    if model is None:
        return
    
    # 2. 创建输出目录
    os.makedirs("exported_models", exist_ok=True)
    
    success_count = 0
    
    # 3. 导出ONNX格式
    if exporter.export_to_onnx(model):
        success_count += 1
    
    # 4. 导出TorchScript格式
    if exporter.export_to_torchscript(model):
        success_count += 1
    
    # 5. 创建模型信息文件
    exporter.create_model_info(model)
    
    # 6. 创建C++头文件
    exporter.create_cpp_header()
    
    print("\n" + "=" * 50)
    print(f"📊 导出完成！成功导出 {success_count}/2 种格式")
    print("\n📁 导出文件列表:")
    
    export_dir = "exported_models"
    if os.path.exists(export_dir):
        for file in os.listdir(export_dir):
            file_path = os.path.join(export_dir, file)
            size = os.path.getsize(file_path) / (1024 * 1024)  # MB
            print(f"  • {file} ({size:.2f} MB)")
    
    print("\n💡 使用说明:")
    print("  1. ONNX格式 (.onnx): 推荐用于生产环境，跨平台兼容性好")
    print("  2. TorchScript格式 (.pt): 适用于PyTorch C++前端")
    print("  3. 模型信息 (model_info.json): 包含集成所需的详细信息")
    print("  4. C++头文件 (AIModelInterface.h): 集成参考接口")
    print("\n🔄 下一步:")
    print("  • 将导出的模型文件拷贝到C++项目中")
    print("  • 根据AIModelInterface.h实现推理接口")
    print("  • 在新的AI集成分支中开始集成工作")

if __name__ == "__main__":
    main()
