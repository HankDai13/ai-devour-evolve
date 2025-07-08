"""
创建一个简单的ONNX模型用于GoBigger AI演示
这个脚本会生成一个简单的神经网络模型，接受400维输入，输出3维动作
"""

import torch
import torch.nn as nn
import torch.onnx
import numpy as np
import os

class SimpleGoBiggerModel(nn.Module):
    def __init__(self, input_size=400, hidden_size=256, output_size=3):
        super(SimpleGoBiggerModel, self).__init__()
        
        self.network = nn.Sequential(
            # 输入层到第一隐藏层
            nn.Linear(input_size, hidden_size),
            nn.ReLU(),
            nn.Dropout(0.2),
            
            # 第二隐藏层
            nn.Linear(hidden_size, 128),
            nn.ReLU(),
            nn.Dropout(0.2),
            
            # 第三隐藏层
            nn.Linear(128, 64),
            nn.ReLU(),
            
            # 输出层
            nn.Linear(64, output_size)
        )
        
    def forward(self, x):
        output = self.network(x)
        
        # 对输出进行处理
        # dx, dy: tanh激活，输出范围[-1, 1]
        dx = torch.tanh(output[:, 0:1])
        dy = torch.tanh(output[:, 1:2])
        
        # action_type: softmax后取最大值的索引，转换为浮点数
        action_logits = output[:, 2:3]
        # 简化：直接使用tanh，然后映射到[0, 2]
        action_type = torch.sigmoid(action_logits) * 2  # 输出范围[0, 2]
        
        return torch.cat([dx, dy, action_type], dim=1)

def create_demo_model():
    """创建并保存演示模型"""
    
    # 创建模型
    model = SimpleGoBiggerModel()
    
    # 随机初始化权重（实际使用时需要训练）
    # 这里我们手动设置一些合理的权重
    with torch.no_grad():
        for param in model.parameters():
            if len(param.shape) > 1:  # 权重矩阵
                nn.init.xavier_uniform_(param)
            else:  # 偏置
                nn.init.zeros_(param)
    
    # 设置为评估模式
    model.eval()
    
    # 创建示例输入
    batch_size = 1
    input_size = 400
    dummy_input = torch.randn(batch_size, input_size)
    
    # 确保输出目录存在
    output_dir = "assets/ai_models"
    os.makedirs(output_dir, exist_ok=True)
    
    # 导出为ONNX格式
    output_path = os.path.join(output_dir, "simple_gobigger_demo.onnx")
    
    torch.onnx.export(
        model,                          # 模型
        dummy_input,                    # 示例输入
        output_path,                    # 输出路径
        export_params=True,             # 导出参数
        opset_version=11,               # ONNX opset版本
        do_constant_folding=True,       # 是否执行常量折叠优化
        input_names=['observation'],    # 输入名称
        output_names=['action'],        # 输出名称
        dynamic_axes={
            'observation': {0: 'batch_size'},  # 批处理大小可变
            'action': {0: 'batch_size'}
        }
    )
    
    print(f"ONNX模型已保存到: {output_path}")
    
    # 验证模型
    import onnx
    onnx_model = onnx.load(output_path)
    onnx.checker.check_model(onnx_model)
    print("ONNX模型验证通过!")
    
    # 测试推理
    test_input = np.random.randn(1, 400).astype(np.float32)
    
    # 使用PyTorch模型测试
    with torch.no_grad():
        torch_output = model(torch.from_numpy(test_input))
        print(f"PyTorch输出: {torch_output.numpy()}")
    
    # 如果安装了onnxruntime，也可以测试ONNX推理
    try:
        import onnxruntime as ort
        
        # 创建推理会话
        ort_session = ort.InferenceSession(output_path)
        
        # 运行推理
        ort_inputs = {ort_session.get_inputs()[0].name: test_input}
        ort_outputs = ort_session.run(None, ort_inputs)
        
        print(f"ONNX Runtime输出: {ort_outputs[0]}")
        print("ONNX模型测试成功!")
        
    except ImportError:
        print("注意: 未安装onnxruntime，跳过ONNX推理测试")
    
    return output_path

if __name__ == "__main__":
    model_path = create_demo_model()
    print(f"\n演示模型创建完成: {model_path}")
    print("\n模型说明:")
    print("- 输入: 400维观察向量")
    print("- 输出: 3维动作向量 [dx, dy, action_type]")
    print("  - dx, dy: 移动方向 [-1.0, 1.0]")
    print("  - action_type: 动作类型 [0.0, 2.0] (0=移动, 1=分裂, 2=喷射)")
    print("\n注意: 这只是一个随机初始化的演示模型，实际使用需要训练!")
