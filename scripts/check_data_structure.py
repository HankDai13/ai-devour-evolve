#!/usr/bin/env python3
"""
检查数据结构
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

print("🎮 检查数据结构...")

engine = gobigger_env.GameEngine()
obs = engine.reset()
player = obs.player_states[0]

print(f"PlayerState 类型: {type(player)}")
print(f"clone 类型: {type(player.clone)}")
print(f"clone 长度: {len(player.clone)}")
print(f"food 类型: {type(player.food)}")
print(f"food 长度: {len(player.food)}")

if player.clone:
    print(f"clone[0] 类型: {type(player.clone[0])}")
    print(f"clone[0] 内容: {dir(player.clone[0])}")
    
if player.food:
    print(f"food[0] 类型: {type(player.food[0])}")
    print(f"food[0] 内容: {dir(player.food[0])}")

print(f"\nGlobalState:")
print(f"global_state 类型: {type(obs.global_state)}")
print(f"global_state 内容: {dir(obs.global_state)}")
