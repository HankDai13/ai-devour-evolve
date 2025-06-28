#!/usr/bin/env python3
"""
测试奖励函数变化性
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

print("Testing reward variation...")

env = GoBiggerEnv()
rewards = []

for episode in range(5):
    obs, info = env.reset()
    total_reward = 0
    
    for step in range(100):
        action = env.action_space.sample()
        obs, reward, done, trunc, info = env.step(action)
        total_reward += reward
        
        if done or trunc:
            break
    
    rewards.append(total_reward)
    print(f"Episode {episode+1}: Total reward = {total_reward:.3f}")

env.close()

print(f"\nReward Statistics:")
print(f"  Mean: {sum(rewards)/len(rewards):.3f}")
print(f"  Min: {min(rewards):.3f}")
print(f"  Max: {max(rewards):.3f}")
print(f"  Range: {max(rewards) - min(rewards):.3f}")

if max(rewards) - min(rewards) > 0.1:
    print("✅ SUCCESS: Rewards show good variation!")
else:
    print("❌ WARNING: Rewards show limited variation")
