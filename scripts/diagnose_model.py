"""
ONNX模型诊断工具
用于分析和修复RL训练模型的兼容性问题
"""

import onnx
import onnxruntime as ort
import numpy as np
import sys
import os
from pathlib import Path

def diagnose_onnx_model(model_path):
    """诊断ONNX模型的详细信息"""
    
    print(f"🔍 诊断ONNX模型: {model_path}")
    print("=" * 60)
    
    # 检查文件是否存在
    if not os.path.exists(model_path):
        print(f"❌ 错误: 模型文件不存在: {model_path}")
        return False
    
    # 获取文件信息
    file_size = os.path.getsize(model_path)
    print(f"📁 文件大小: {file_size:,} 字节 ({file_size/1024/1024:.2f} MB)")
    
    try:
        # 加载和验证ONNX模型
        print("\n🔧 加载ONNX模型...")
        model = onnx.load(model_path)
        
        print("✅ ONNX模型加载成功")
        
        # 验证模型
        print("\n🔍 验证模型结构...")
        try:
            onnx.checker.check_model(model)
            print("✅ 模型结构验证通过")
        except Exception as e:
            print(f"⚠️  模型结构验证警告: {e}")
        
        # 获取模型信息
        graph = model.graph
        print(f"\n📊 模型信息:")
        print(f"   模型名称: {model.graph.name if model.graph.name else 'Unnamed'}")
        print(f"   ONNX版本: {model.opset_import[0].version if model.opset_import else 'Unknown'}")
        print(f"   节点数量: {len(graph.node)}")
        
        # 分析输入
        print(f"\n📥 输入信息 ({len(graph.input)}个):")
        for i, input_tensor in enumerate(graph.input):
            print(f"   输入 {i}:")
            print(f"     名称: {input_tensor.name}")
            print(f"     类型: {input_tensor.type.tensor_type.elem_type}")
            
            # 获取形状
            shape = []
            for dim in input_tensor.type.tensor_type.shape.dim:
                if dim.dim_value:
                    shape.append(dim.dim_value)
                elif dim.dim_param:
                    shape.append(f"'{dim.dim_param}'")
                else:
                    shape.append("?")
            print(f"     形状: {shape}")
        
        # 分析输出
        print(f"\n📤 输出信息 ({len(graph.output)}个):")
        for i, output_tensor in enumerate(graph.output):
            print(f"   输出 {i}:")
            print(f"     名称: {output_tensor.name}")
            print(f"     类型: {output_tensor.type.tensor_type.elem_type}")
            
            # 获取形状
            shape = []
            for dim in output_tensor.type.tensor_type.shape.dim:
                if dim.dim_value:
                    shape.append(dim.dim_value)
                elif dim.dim_param:
                    shape.append(f"'{dim.dim_param}'")
                else:
                    shape.append("?")
            print(f"     形状: {shape}")
        
    except Exception as e:
        print(f"❌ ONNX模型加载失败: {e}")
        return False
    
    # 测试ONNX Runtime
    print(f"\n🚀 测试ONNX Runtime推理...")
    try:
        # 创建推理会话
        session = ort.InferenceSession(model_path)
        
        print("✅ ONNX Runtime会话创建成功")
        
        # 获取输入输出信息
        input_info = session.get_inputs()[0]
        output_info = session.get_outputs()[0]
        
        print(f"\n🔧 Runtime信息:")
        print(f"   输入名称: {input_info.name}")
        print(f"   输入形状: {input_info.shape}")
        print(f"   输入类型: {input_info.type}")
        print(f"   输出名称: {output_info.name}")
        print(f"   输出形状: {output_info.shape}")
        print(f"   输出类型: {output_info.type}")
        
        # 创建测试输入
        input_shape = input_info.shape
        
        # 处理动态形状
        test_shape = []
        for dim in input_shape:
            if isinstance(dim, str) or dim == -1:
                test_shape.append(1)  # 动态维度设为1
            else:
                test_shape.append(dim)
        
        # 如果第二个维度还是动态的，假设为400（观察空间大小）
        if len(test_shape) > 1 and test_shape[1] == 1:
            test_shape[1] = 400
        
        print(f"   测试输入形状: {test_shape}")
        
        # 创建随机测试数据
        test_input = np.random.randn(*test_shape).astype(np.float32)
        
        # 运行推理
        result = session.run([output_info.name], {input_info.name: test_input})
        
        print(f"✅ 推理测试成功!")
        print(f"   输出形状: {result[0].shape}")
        print(f"   输出样例: {result[0].flatten()[:5]}")  # 显示前5个值
        
        return True
        
    except Exception as e:
        print(f"❌ ONNX Runtime测试失败: {e}")
        print(f"   详细错误信息: {type(e).__name__}: {str(e)}")
        return False

def fix_model_compatibility(model_path, output_path=None):
    """尝试修复模型兼容性问题"""
    
    if output_path is None:
        name, ext = os.path.splitext(model_path)
        output_path = f"{name}_fixed{ext}"
    
    print(f"\n🔧 尝试修复模型兼容性...")
    print(f"输入: {model_path}")
    print(f"输出: {output_path}")
    
    try:
        # 加载模型
        model = onnx.load(model_path)
        
        # 简化模型（移除不必要的节点）
        from onnx import optimizer
        
        # 应用优化passes
        passes = [
            'eliminate_deadend',
            'eliminate_identity',
            'eliminate_nop_dropout',
            'eliminate_nop_pad',
            'eliminate_unused_initializer',
            'extract_constant_to_initializer',
            'fuse_add_bias_into_conv',
            'fuse_bn_into_conv',
            'fuse_consecutive_concats',
            'fuse_consecutive_log_softmax',
            'fuse_consecutive_reduce_unsqueeze',
            'fuse_consecutive_squeezes',
            'fuse_consecutive_transposes',
            'fuse_matmul_add_bias_into_gemm',
            'fuse_pad_into_conv',
            'fuse_transpose_into_gemm',
            'lift_lexical_references',
        ]
        
        optimized_model = optimizer.optimize(model, passes)
        
        # 保存优化后的模型
        onnx.save(optimized_model, output_path)
        
        print(f"✅ 模型优化完成: {output_path}")
        
        # 验证修复后的模型
        if diagnose_onnx_model(output_path):
            print(f"✅ 修复后的模型验证通过!")
            return True
        else:
            print(f"❌ 修复后的模型仍有问题")
            return False
            
    except Exception as e:
        print(f"❌ 模型修复失败: {e}")
        return False

def main():
    if len(sys.argv) < 2:
        print("使用方法: python diagnose_model.py <model_path> [--fix]")
        print("示例:")
        print("  python diagnose_model.py my_model.onnx")
        print("  python diagnose_model.py my_model.onnx --fix")
        return
    
    model_path = sys.argv[1]
    should_fix = "--fix" in sys.argv
    
    # 诊断模型
    success = diagnose_onnx_model(model_path)
    
    if not success and should_fix:
        print(f"\n🔧 模型有问题，尝试修复...")
        fix_model_compatibility(model_path)
    elif success:
        print(f"\n✅ 模型完全兼容！可以在GoBigger AI中使用")
    else:
        print(f"\n❌ 模型有兼容性问题，建议使用 --fix 参数尝试修复")
        print(f"\n💡 常见问题和解决方案:")
        print(f"   1. 动态输入形状: 确保模型支持batch_size=1")
        print(f"   2. 输入大小不匹配: 检查观察空间是否为400维")
        print(f"   3. 输出格式问题: 确保输出为3维 [dx, dy, action_type]")
        print(f"   4. ONNX版本问题: 使用opset_version=11导出")

if __name__ == "__main__":
    main()
