#!/usr/bin/env python3
"""
调试游戏状态：检查分数、移动、食物碰撞
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

print("🎮 调试游戏状态...")

engine = gobigger_env.GameEngine()
obs = engine.reset()
player = obs.player_states[0]

print(f"📊 初始状态:")
print(f"   分数: {player.score}")
print(f"   克隆球数量: {len(player.clone)}")
if player.clone:
    clone = player.clone[0]  # 主克隆球
    print(f"   位置: ({clone.position.x():.2f}, {clone.position.y():.2f})")
    print(f"   半径: {clone.radius}")
    print(f"   速度: ({clone.velocity.x():.4f}, {clone.velocity.y():.4f})")
print(f"   食物数量: {len(player.food)}")

# 找到最近的食物
if player.food:
    closest_food = None
    min_distance = float('inf')
    clone_pos = player.clone[0].position if player.clone else None
    if clone_pos:
        for food in player.food:
            distance = ((food.position.x() - clone_pos.x())**2 + (food.position.y() - clone_pos.y())**2)**0.5
            if distance < min_distance:
                min_distance = distance
                closest_food = food
        
        print(f"🍎 最近食物:")
        print(f"   位置: ({closest_food.position.x():.2f}, {closest_food.position.y():.2f})")
        print(f"   距离: {min_distance:.2f}")
        print(f"   食物半径: {closest_food.radius}")
        if player.clone:
            print(f"   碰撞阈值: {player.clone[0].radius + closest_food.radius}")

print("\n🏃 测试移动...")

# 测试向右移动
action = gobigger_env.Action(1.0, 0.0, 0)
prev_pos = player.clone[0].position if player.clone else None
prev_score = player.score

for i in range(10):
    obs = engine.step(action)
    player = obs.player_states[0]
    
    if player.clone and prev_pos:
        current_pos = player.clone[0].position
        distance_moved = ((current_pos.x() - prev_pos.x())**2 + (current_pos.y() - prev_pos.y())**2)**0.5
        score_change = player.score - prev_score
        
        print(f"帧{i+1:2d}: 位置({current_pos.x():8.2f}, {current_pos.y():8.2f}), 移动距离{distance_moved:6.2f}, 分数{player.score:6.1f} (+{score_change:4.1f})")
        
        prev_pos = current_pos
        prev_score = player.score
        
        # 如果分数变化了，说明吃到了食物
        if score_change > 0:
            print(f"   ✅ 吃到食物！分数增加了 {score_change}")

print(f"\n📈 最终状态:")
if obs.player_states[0].clone:
    final_pos = obs.player_states[0].clone[0].position
    initial_pos = obs.player_states[0].clone[0].position  # 这个需要记录初始位置
    print(f"   最终位置: ({final_pos.x():.2f}, {final_pos.y():.2f})")
print(f"   分数变化: {obs.player_states[0].score - 1000.0}")
print(f"   克隆球数量: {len(obs.player_states[0].clone)}")
print(f"   可见食物: {len(obs.player_states[0].food)}")
