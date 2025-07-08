#!/usr/bin/env python3
"""
快速验证导出的AI模型是否工作正常
"""

import torch
import numpy as np
import json
import os

def test_pytorch_model():
    """测试PyTorch TorchScript模型"""
    model_path = "assets/ai_models/exported_models/ai_model_traced.pt"
    
    if not os.path.exists(model_path):
        print(f"❌ Model file not found: {model_path}")
        return False
    
    try:
        print(f"🔄 Loading PyTorch model: {model_path}")
        model = torch.jit.load(model_path)
        model.eval()
        
        # 创建测试输入 (1批次, 400维观测)
        test_input = torch.randn(1, 400)
        print(f"📥 Test input shape: {test_input.shape}")
        
        # 执行推理
        with torch.no_grad():
            output = model(test_input)
        
        # 处理可能的元组输出
        if isinstance(output, tuple):
            output = output[0]  # 取第一个元素
        
        print(f"📤 Output shape: {output.shape}")
        print(f"📤 Output values: {output.squeeze().tolist()}")
        
        # 解析输出
        dx, dy, action_type_raw = output.squeeze().tolist()
        
        # 处理输出
        dx = max(-1.0, min(1.0, dx))
        dy = max(-1.0, min(1.0, dy))
        
        if action_type_raw >= 1.5:
            action_type = "EJECT"
        elif action_type_raw >= 0.5:
            action_type = "SPLIT"
        else:
            action_type = "MOVE"
        
        print(f"✅ PyTorch model test successful!")
        print(f"   🎯 Action: {action_type}")
        print(f"   📍 Direction: dx={dx:.3f}, dy={dy:.3f}")
        print(f"   🔢 Raw action type: {action_type_raw:.3f}")
        
        return True
        
    except Exception as e:
        print(f"❌ PyTorch model test failed: {e}")
        return False

def test_onnx_model():
    """测试ONNX模型"""
    model_path = "assets/ai_models/exported_models/ai_model.onnx"
    
    if not os.path.exists(model_path):
        print(f"❌ ONNX model file not found: {model_path}")
        return False
    
    try:
        import onnxruntime as ort
        
        print(f"🔄 Loading ONNX model: {model_path}")
        session = ort.InferenceSession(model_path)
        
        # 创建测试输入
        test_input = np.random.randn(1, 400).astype(np.float32)
        print(f"📥 Test input shape: {test_input.shape}")
        
        # 执行推理
        input_name = session.get_inputs()[0].name
        output_name = session.get_outputs()[0].name
        
        result = session.run([output_name], {input_name: test_input})
        output = result[0]
        
        print(f"📤 Output shape: {output.shape}")
        print(f"📤 Output values: {output.squeeze().tolist()}")
        
        # 解析输出
        dx, dy, action_type_raw = output.squeeze().tolist()
        
        # 处理输出
        dx = max(-1.0, min(1.0, dx))
        dy = max(-1.0, min(1.0, dy))
        
        if action_type_raw >= 1.5:
            action_type = "EJECT"
        elif action_type_raw >= 0.5:
            action_type = "SPLIT"
        else:
            action_type = "MOVE"
        
        print(f"✅ ONNX model test successful!")
        print(f"   🎯 Action: {action_type}")
        print(f"   📍 Direction: dx={dx:.3f}, dy={dy:.3f}")
        print(f"   🔢 Raw action type: {action_type_raw:.3f}")
        
        return True
        
    except ImportError:
        print("⚠️ ONNX Runtime not installed, skipping ONNX test")
        print("   Install with: pip install onnxruntime")
        return False
    except Exception as e:
        print(f"❌ ONNX model test failed: {e}")
        return False

def test_model_info():
    """验证模型信息文件"""
    info_path = "assets/ai_models/exported_models/model_info.json"
    
    if not os.path.exists(info_path):
        print(f"❌ Model info file not found: {info_path}")
        return False
    
    try:
        print(f"🔄 Loading model info: {info_path}")
        with open(info_path, 'r') as f:
            info = json.load(f)
        
        print(f"✅ Model info loaded successfully!")
        print(f"   📋 Model type: {info.get('model_type', 'Unknown')}")
        print(f"   📏 Observation space: {info.get('observation_space_size', 'Unknown')}")
        print(f"   🎮 Action space: {info.get('action_space', {})}")
        print(f"   📝 Notes: {len(info.get('usage_notes', []))} usage notes")
        
        return True
        
    except Exception as e:
        print(f"❌ Model info test failed: {e}")
        return False

def main():
    print("🤖 AI Model Validation Test")
    print("=" * 50)
    
    # 检查当前目录
    current_dir = os.getcwd()
    print(f"📁 Current directory: {current_dir}")
    
    if not os.path.exists("assets/ai_models/exported_models"):
        print("❌ Models directory not found!")
        print("   Please run this script from the project root directory")
        return
    
    print("\n🧪 Running model tests...")
    
    # 测试模型信息
    print("\n1️⃣ Testing model info...")
    test_model_info()
    
    # 测试PyTorch模型
    print("\n2️⃣ Testing PyTorch model...")
    pytorch_ok = test_pytorch_model()
    
    # 测试ONNX模型
    print("\n3️⃣ Testing ONNX model...")
    onnx_ok = test_onnx_model()
    
    # 总结
    print("\n📊 Test Summary:")
    print("=" * 50)
    print(f"PyTorch Model: {'✅ PASS' if pytorch_ok else '❌ FAIL'}")
    print(f"ONNX Model: {'✅ PASS' if onnx_ok else '❌ FAIL'}")
    
    if pytorch_ok:
        print("\n🎉 At least PyTorch model is working!")
        print("   Ready for C++ integration with LibTorch")
    else:
        print("\n⚠️ PyTorch model failed!")
        print("   Please check model export process")

if __name__ == "__main__":
    main()
