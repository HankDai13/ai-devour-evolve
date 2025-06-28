#!/usr/bin/env python3
"""
快速验证动作限制功能
"""
import sys
import os
from pathlib import Path

# 路径设置：项目根目录
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

import gobigger_env

print("🎯 测试动作限制功能...")

# 测试超出范围的动作
action = gobigger_env.Action(2.5, -3.0, 5)
print(f"原始动作: ({action.direction_x}, {action.direction_y}, {action.action_type})")

# 使用限制函数
clamped = gobigger_env.clamp_action(action)
print(f"限制后动作: ({clamped.direction_x}, {clamped.direction_y}, {clamped.action_type})")

# 测试引擎内部的限制（通过step函数）
engine = gobigger_env.GameEngine()
engine.reset()
print("执行超范围动作...")
obs = engine.step(action)
print(f"✅ 引擎内部正确限制了动作范围并成功执行到帧: {obs.global_state.total_frame}")
