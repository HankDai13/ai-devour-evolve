#!/usr/bin/env python3
"""
详细调试碰撞检测流程
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
    x = norm_x * border_width - border_width / 2
    y = norm_y * border_height - border_height / 2
    return x, y

def denormalize_radius(norm_radius, border_size):
    return norm_radius * border_size / 2

print("🔍 详细调试碰撞检测流程...")

engine = gobigger_env.GameEngine()
obs = engine.reset()
player = obs.player_states[0]
border_width, border_height = obs.global_state.border

# 分析初始状态
main_clone = player.clone[0]
player_x, player_y = denormalize_position(main_clone[0], main_clone[1], border_width, border_height)
player_radius = denormalize_radius(main_clone[2], max(border_width, border_height))
player_score = player.score

print(f"🎯 玩家初始状态:")
print(f"   位置: ({player_x:.2f}, {player_y:.2f})")
print(f"   半径: {player_radius:.2f}")
print(f"   分数: {player_score}")

# 计算食物分数比例
food_score = 100  # 从GoBiggerConfig得到
eat_ratio = 1.3
can_eat_threshold = food_score * eat_ratio
print(f"   食物分数: {food_score}")
print(f"   吃掉阈值: {can_eat_threshold}")
print(f"   可以吃食物: {'✅' if player_score >= can_eat_threshold else '❌'}")

# 找最近食物，精确计算位置
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
    collision_threshold = player_radius + food_radius
    
    print(f"\n🍎 最近食物:")
    print(f"   位置: ({food_x:.2f}, {food_y:.2f})")
    print(f"   半径: {food_radius:.2f}")
    print(f"   距离: {min_distance:.2f}")
    print(f"   碰撞阈值: {collision_threshold:.2f}")
    print(f"   已碰撞: {'✅' if min_distance <= collision_threshold else '❌'}")

    # 移动朝食物
    direction_x = food_x - player_x
    direction_y = food_y - player_y
    direction_length = (direction_x**2 + direction_y**2)**0.5
    
    if direction_length > 0:
        direction_x /= direction_length
        direction_y /= direction_length
        
        print(f"\n🏃 开始移动朝食物...")
        action = gobigger_env.Action(direction_x, direction_y, 0)
        
        for i in range(15):
            prev_score = obs.player_states[0].score
            obs = engine.step(action)
            player = obs.player_states[0]
            
            if player.clone:
                main_clone = player.clone[0]
                current_x, current_y = denormalize_position(main_clone[0], main_clone[1], border_width, border_height)
                distance_to_food = ((food_x - current_x)**2 + (food_y - current_y)**2)**0.5
                
                score_change = player.score - prev_score
                is_colliding = distance_to_food <= collision_threshold
                
                print(f"帧{i+1:2d}: 距离{distance_to_food:6.2f}, 碰撞{is_colliding}, 分数{player.score:6.1f}, 变化{score_change:+5.1f}")
                
                if score_change > 0:
                    print(f"      ✅ 成功吃到食物！")
                    break
                elif is_colliding and score_change == 0:
                    print(f"      ⚠️  已碰撞但分数未变化！可能碰撞检测有问题")

print(f"\n📋 诊断总结:")
final_score = obs.player_states[0].score
if final_score > 1000.0:
    print("   ✅ 碰撞检测正常工作")
else:
    print("   ❌ 碰撞检测有问题，可能原因：")
    print("      1. QuadTree查询未找到食物")
    print("      2. collidesWith()方法返回false")
    print("      3. canEat()方法返回false")
    print("      4. eat()方法没有正确执行")
    print("      5. updateGame()没有调用碰撞检测")
