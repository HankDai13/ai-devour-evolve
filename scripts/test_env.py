#!/usr/bin/env python3
"""
测试 gobigger_env Python 模块
"""
import sys
import os
from pathlib import Path

# 路径设置：定位到项目根目录
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{build_dir};{os.environ['PATH']}"

try:
    import gobigger_env
    print(f"✅ 成功导入 gobigger_env 模块!")
    print(f"📦 模块版本: {gobigger_env.__version__}")
    print(f"👨‍💻 作者: {gobigger_env.__author__}")
    
    # 测试创建 GameEngine
    print("\n🎮 测试 GameEngine...")
    engine = gobigger_env.GameEngine()
    print("✅ GameEngine 创建成功!")
    
    # 测试创建 Action
    print("\n🎯 测试 Action...")
    action = gobigger_env.Action(1.0, 0.5, 1)
    print(f"✅ Action 创建成功: direction_x={action.direction_x}, direction_y={action.direction_y}, action_type={action.action_type}")
    
    # 测试 create_action 辅助函数
    action2 = gobigger_env.create_action(0.8, -0.3, 2)
    print(f"✅ create_action 函数工作正常: direction_x={action2.direction_x}, direction_y={action2.direction_y}, action_type={action2.action_type}")
    
    # 测试重置环境
    print("\n🔄 测试环境重置...")
    obs = engine.reset()
    print("✅ 环境重置成功!")
    print(f"🌐 Global state - total_frame: {obs.global_state.total_frame}")
    print(f"👥 Player states 数量: {len(obs.player_states)}")
    
    # 测试执行动作
    print("\n▶️ 测试执行动作...")
    obs2 = engine.step(action)
    print("✅ 动作执行成功!")
    print(f"🌐 执行后 total_frame: {obs2.global_state.total_frame}")
    
    # 测试游戏状态查询
    print("\n❓ 测试游戏状态查询...")
    is_done = engine.is_done()
    is_running = engine.is_game_running()
    total_frames = engine.get_total_frames()
    print(f"✅ 游戏是否结束: {is_done}")
    print(f"✅ 游戏是否运行: {is_running}")
    print(f"✅ 总帧数: {total_frames}")
    
    print("\n🎉 所有测试通过！Qt类型自动转换工作正常！")
    
except ImportError as e:
    print(f"❌ 导入失败: {e}")
    sys.exit(1)
except Exception as e:
    print(f"❌ 测试失败: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
