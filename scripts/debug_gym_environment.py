#!/usr/bin/env python3
"""
详细调试Gymnasium环境中的食物吞噬
"""
import sys
import os
from pathlib import Path

# 路径设置
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

from gobigger_gym_env import GoBiggerEnv
import numpy as np

def denormalize_position(norm_x, norm_y, border_width=6000, border_height=6000):
    x = norm_x * border_width - border_width / 2
    y = norm_y * border_height - border_height / 2
    return x, y

print("=== Gymnasium环境食物吞噬调试 ===")

env = GoBiggerEnv()
obs, info = env.reset()

# 获取初始状态
if env.current_obs and env.current_obs.player_states:
    ps = list(env.current_obs.player_states.values())[0]
    print(f"初始分数: {ps.score}")
    
    # 分析玩家和食物位置
    if ps.clone and ps.food:
        player_norm = ps.clone[0][:2]  # 取前两个元素作为位置
        player_x, player_y = denormalize_position(player_norm[0], player_norm[1])
        
        # 找最近的食物
        closest_food = None
        min_distance = float('inf')
        
        for food in ps.food[:5]:  # 只检查前5个食物
            food_x, food_y = denormalize_position(food[0], food[1])
            distance = ((food_x - player_x)**2 + (food_y - player_y)**2)**0.5
            
            if distance < min_distance:
                min_distance = distance
                closest_food = (food_x, food_y)
        
        if closest_food:
            print(f"玩家位置: ({player_x:.1f}, {player_y:.1f})")
            print(f"最近食物: ({closest_food[0]:.1f}, {closest_food[1]:.1f})")
            print(f"距离: {min_distance:.1f}")
            
            # 计算朝食物的方向
            direction_x = closest_food[0] - player_x
            direction_y = closest_food[1] - player_y
            length = (direction_x**2 + direction_y**2)**0.5
            
            if length > 0:
                direction_x /= length
                direction_y /= length
                
                print(f"移动方向: ({direction_x:.3f}, {direction_y:.3f})")
                
                # 朝食物移动
                for step in range(50):
                    action = np.array([direction_x, direction_y, 0])
                    obs, reward, done, trunc, info = env.step(action)
                    
                    # 检查分数变化
                    if env.current_obs and env.current_obs.player_states:
                        current_ps = list(env.current_obs.player_states.values())[0]
                        score_change = current_ps.score - 1000.0
                        
                        if step % 10 == 0 or score_change > 0:
                            print(f"步骤{step+1:2d}: 分数={current_ps.score:.1f} (+{score_change:.1f}), 奖励={reward:.4f}")
                        
                        if score_change > 0:
                            print("✅ 成功吃到食物！")
                            break
                    
                    if done or trunc:
                        break
                
                if current_ps.score == 1000.0:
                    print("❌ 没有吃到食物")

env.close()
