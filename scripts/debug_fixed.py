#!/usr/bin/env python3
"""
修复版本：检查游戏机制
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

def denormalize_position(norm_x, norm_y, border_width, border_height):
    """反归一化位置坐标"""
    x = norm_x * border_width - border_width / 2
    y = norm_y * border_height - border_height / 2
    return x, y

def denormalize_radius(norm_radius, border_size):
    """反归一化半径"""
    return norm_radius * border_size / 2

print("🎮 检查游戏机制...")

engine = gobigger_env.GameEngine()
obs = engine.reset()
player = obs.player_states[0]

# 获取边界信息
border_width, border_height = obs.global_state.border
print(f"🗺️ 地图大小: {border_width} x {border_height}")

# 分析玩家状态
print(f"📊 初始状态:")
print(f"   分数: {player.score}")
print(f"   克隆球数量: {len(player.clone)}")
print(f"   可见食物数量: {len(player.food)}")

# 分析主克隆球
if player.clone:
    main_clone = player.clone[0]  # [x_norm, y_norm, radius_norm, ...]
    x, y = denormalize_position(main_clone[0], main_clone[1], border_width, border_height)
    radius = denormalize_radius(main_clone[2], max(border_width, border_height))
    
    print(f"🎯 主克隆球:")
    print(f"   归一化位置: ({main_clone[0]:.4f}, {main_clone[1]:.4f})")
    print(f"   实际位置: ({x:.2f}, {y:.2f})")
    print(f"   归一化半径: {main_clone[2]:.4f}")
    print(f"   实际半径: {radius:.2f}")

# 分析最近的食物
if player.food:
    distances = []
    for i, food in enumerate(player.food[:5]):  # 只看前5个食物
        food_x, food_y = denormalize_position(food[0], food[1], border_width, border_height)
        food_radius = denormalize_radius(food[2], max(border_width, border_height))
        
        # 计算距离
        main_x, main_y = denormalize_position(main_clone[0], main_clone[1], border_width, border_height)
        distance = ((food_x - main_x)**2 + (food_y - main_y)**2)**0.5
        collision_threshold = radius + food_radius
        
        distances.append((distance, collision_threshold, i))
        
        if i < 3:  # 显示前3个食物
            print(f"🍎 食物{i}:")
            print(f"   位置: ({food_x:.2f}, {food_y:.2f})")
            print(f"   半径: {food_radius:.2f}")
            print(f"   距离: {distance:.2f}")
            print(f"   碰撞阈值: {collision_threshold:.2f}")
            print(f"   {'✅ 可以吃到' if distance <= collision_threshold else '❌ 距离太远'}")
    
    # 找到最近的食物
    distances.sort()
    closest_distance, closest_threshold, closest_idx = distances[0]
    print(f"\n🎯 最近食物距离: {closest_distance:.2f}, 阈值: {closest_threshold:.2f}")

print("\n🏃 测试移动10帧...")

# 测试移动
action = gobigger_env.Action(1.0, 0.0, 0)  # 向右移动
initial_score = player.score

for i in range(10):
    obs = engine.step(action)
    player = obs.player_states[0]
    
    if player.clone:
        main_clone = player.clone[0]
        x, y = denormalize_position(main_clone[0], main_clone[1], border_width, border_height)
        score_change = player.score - initial_score
        
        print(f"帧{i+1:2d}: 位置({x:8.2f}, {y:8.2f}), 分数{player.score:6.1f} (+{score_change:4.1f})")
        
        if score_change > 0:
            print(f"      ✅ 吃到食物！分数增加了 {score_change}")

print(f"\n📈 结果分析:")
print(f"   最终分数: {obs.player_states[0].score}")
print(f"   分数变化: {obs.player_states[0].score - 1000.0}")
print(f"   原因分析: {'分数未变化，可能是移动距离不够或碰撞检测有问题' if obs.player_states[0].score == 1000.0 else '正常吃到食物'}")
