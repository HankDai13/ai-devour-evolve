#!/usr/bin/env python3
"""
测试朝食物方向移动
"""
import sys
import os
from pathlib import Path
import math

# 路径设置
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

import gobigger_env

def denormalize_position(norm_x, norm_y, border_width, border_height):
    """反归一化位置坐标"""
    x = norm_x * border_width - border_width / 2
    y = norm_y * border_height - border_height / 2
    return x, y

def denormalize_radius(norm_radius, border_size):
    """反归一化半径"""
    return norm_radius * border_size / 2

print("🎮 测试朝食物方向移动...")

engine = gobigger_env.GameEngine()
obs = engine.reset()
player = obs.player_states[0]
border_width, border_height = obs.global_state.border

# 分析玩家和食物位置
main_clone = player.clone[0]
player_x, player_y = denormalize_position(main_clone[0], main_clone[1], border_width, border_height)
player_radius = denormalize_radius(main_clone[2], max(border_width, border_height))

print(f"🎯 玩家位置: ({player_x:.2f}, {player_y:.2f}), 半径: {player_radius:.2f}")

# 找到最近的食物
closest_food = None
min_distance = float('inf')
for food in player.food:
    food_x, food_y = denormalize_position(food[0], food[1], border_width, border_height)
    distance = ((food_x - player_x)**2 + (food_y - player_y)**2)**0.5
    if distance < min_distance:
        min_distance = distance
        closest_food = (food_x, food_y, food[2])

if closest_food:
    food_x, food_y, food_radius_norm = closest_food
    food_radius = denormalize_radius(food_radius_norm, max(border_width, border_height))
    
    print(f"🍎 最近食物: ({food_x:.2f}, {food_y:.2f}), 半径: {food_radius:.2f}")
    print(f"📏 距离: {min_distance:.2f}, 碰撞阈值: {player_radius + food_radius:.2f}")
    
    # 计算朝食物的方向
    direction_x = food_x - player_x
    direction_y = food_y - player_y
    direction_length = (direction_x**2 + direction_y**2)**0.5
    
    if direction_length > 0:
        direction_x /= direction_length
        direction_y /= direction_length
        
        print(f"🧭 移动方向: ({direction_x:.3f}, {direction_y:.3f})")
        
        # 测试朝食物移动
        action = gobigger_env.Action(direction_x, direction_y, 0)
        
        print(f"\n🏃 朝最近食物移动...")
        for i in range(30):  # 增加到30帧
            obs = engine.step(action)
            player = obs.player_states[0]
            
            if player.clone:
                main_clone = player.clone[0]
                current_x, current_y = denormalize_position(main_clone[0], main_clone[1], border_width, border_height)
                
                # 重新计算与食物的距离
                distance_to_food = ((food_x - current_x)**2 + (food_y - current_y)**2)**0.5
                score_change = player.score - 1000.0
                
                print(f"帧{i+1:2d}: 位置({current_x:8.2f}, {current_y:8.2f}), 与食物距离{distance_to_food:6.2f}, 分数{player.score:6.1f} (+{score_change:4.1f})")
                
                if score_change > 0:
                    print(f"      ✅ 成功吃到食物！分数增加了 {score_change}")
                    break
                    
                if distance_to_food < player_radius + food_radius:
                    print(f"      ⚠️  已进入碰撞范围，但分数未变化！")

print(f"\n📈 结果分析:")
print(f"   最终分数: {obs.player_states[0].score}")
print(f"   分数变化: {obs.player_states[0].score - 1000.0}")

if obs.player_states[0].score == 1000.0:
    print("   ❌ 问题：移动正常但无法吃到食物，可能是碰撞检测逻辑有问题")
else:
    print("   ✅ 成功：食物吞噬机制工作正常！")
