#!/usr/bin/env python3
"""
深入检查数据结构
"""
import sys
import os
from pathlib import Path

# 路径设置
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

import gobigger_env

print("🎮 深入检查数据结构...")

engine = gobigger_env.GameEngine()
obs = engine.reset()
player = obs.player_states[0]

print(f"克隆球数据结构:")
if player.clone and len(player.clone) > 0:
    clone_data = player.clone[0]
    print(f"  clone[0] 类型: {type(clone_data)}")
    print(f"  clone[0] 长度: {len(clone_data) if hasattr(clone_data, '__len__') else 'N/A'}")
    if isinstance(clone_data, list) and len(clone_data) > 0:
        print(f"  clone[0][0]: {clone_data[0] if len(clone_data) > 0 else 'empty'}")
        print(f"  clone[0][1]: {clone_data[1] if len(clone_data) > 1 else 'N/A'}")
        print(f"  clone[0][2]: {clone_data[2] if len(clone_data) > 2 else 'N/A'}")
        print(f"  clone[0][3]: {clone_data[3] if len(clone_data) > 3 else 'N/A'}")
        print(f"  clone[0][4]: {clone_data[4] if len(clone_data) > 4 else 'N/A'}")

print(f"\n食物数据结构:")
if player.food and len(player.food) > 0:
    food_data = player.food[0]
    print(f"  food[0] 类型: {type(food_data)}")
    print(f"  food[0] 长度: {len(food_data) if hasattr(food_data, '__len__') else 'N/A'}")
    if isinstance(food_data, list) and len(food_data) > 0:
        print(f"  food[0][0]: {food_data[0] if len(food_data) > 0 else 'empty'}")
        print(f"  food[0][1]: {food_data[1] if len(food_data) > 1 else 'N/A'}")
        print(f"  food[0][2]: {food_data[2] if len(food_data) > 2 else 'N/A'}")
        print(f"  food[0][3]: {food_data[3] if len(food_data) > 3 else 'N/A'}")

print(f"\nGlobalState border:")
print(f"  border 类型: {type(obs.global_state.border)}")
print(f"  border 内容: {obs.global_state.border}")
print(f"  total_frame: {obs.global_state.total_frame}")
