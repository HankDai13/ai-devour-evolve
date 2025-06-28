#!/usr/bin/env python3
"""
测试Monitor与GoBiggerEnv的集成是否正常工作
"""
import sys
import os
from pathlib import Path
import numpy as np

# 路径设置
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

import gobigger_env
from gobigger_gym_env import GoBiggerEnv

try:
    from stable_baselines3.common.monitor import Monitor
    from stable_baselines3.common.vec_env import DummyVecEnv
    STABLE_BASELINES_AVAILABLE = True
except ImportError:
    STABLE_BASELINES_AVAILABLE = False
    print("❌ 需要 stable-baselines3")
    exit(1)

def test_monitor_standalone():
    """测试Monitor单独包装的环境"""
    print("🧪 测试Monitor单独包装的环境...")
    
    config = {
        'max_episode_steps': 500,
        'use_enhanced_reward': True,
        'enhanced_reward_weights': {
            'score_growth': 2.0,
            'efficiency': 1.5,
            'exploration': 0.8,
            'strategic_split': 2.0,
            'food_density': 1.0,
            'survival': 0.02,
            'time_penalty': -0.001,
            'death_penalty': -20.0,
        }
    }
    
    # 创建环境并用Monitor包装
    env = GoBiggerEnv(config)
    env = Monitor(env)
    
    total_episodes = 3
    for episode in range(total_episodes):
        print(f"\n📍 Episode {episode + 1}")
        obs, info = env.reset()
        total_reward = 0
        steps = 0
        
        while True:
            action = env.action_space.sample()
            action[2] = int(action[2])
            
            obs, reward, terminated, truncated, info = env.step(action)
            total_reward += reward
            steps += 1
            
            if terminated or truncated:
                print(f"  Episode结束: 总奖励={total_reward:.3f}, 步数={steps}")
                
                # 检查info中的episode信息
                if 'episode' in info:
                    print(f"  ✅ 发现episode信息: {info['episode']}")
                else:
                    print(f"  ❌ 未发现episode信息，info keys: {list(info.keys())}")
                
                if 'final_score' in info:
                    print(f"  最终分数: {info['final_score']:.0f}")
                
                break
    
    print("\n✅ Monitor单独测试完成")

def test_monitor_with_dummyvecenv():
    """测试Monitor + DummyVecEnv的组合"""
    print("\n🧪 测试Monitor + DummyVecEnv的组合...")
    
    config = {
        'max_episode_steps': 500,
        'use_enhanced_reward': True,
        'enhanced_reward_weights': {
            'score_growth': 2.0,
            'efficiency': 1.5,
            'exploration': 0.8,
            'strategic_split': 2.0,
            'food_density': 1.0,
            'survival': 0.02,
            'time_penalty': -0.001,
            'death_penalty': -20.0,
        }
    }
    
    # 创建环境并用Monitor包装，然后用DummyVecEnv包装
    def make_env():
        env = GoBiggerEnv(config)
        env = Monitor(env)
        return env
    
    env = DummyVecEnv([make_env])
    
    total_episodes = 3
    completed_episodes = 0
    
    obs = env.reset()
    print(f"初始obs shape: {obs.shape}")
    
    for step in range(2000):  # 最多运行2000步
        action = env.action_space.sample()
        obs, rewards, dones, infos = env.step(action)
        
        if dones[0]:  # Episode结束
            completed_episodes += 1
            print(f"\n📍 Episode {completed_episodes} (步数 {step+1})")
            print(f"  奖励: {rewards[0]:.3f}")
            print(f"  info: {infos[0]}")
            
            if 'episode' in infos[0]:
                print(f"  ✅ 发现episode信息: {infos[0]['episode']}")
            else:
                print(f"  ❌ 未发现episode信息")
            
            if completed_episodes >= total_episodes:
                break
            
            obs = env.reset()
    
    print(f"\n✅ DummyVecEnv测试完成，完成了{completed_episodes}个episodes")

if __name__ == "__main__":
    print("🔍 Monitor集成测试")
    print("=" * 50)
    
    test_monitor_standalone()
    test_monitor_with_dummyvecenv()
    
    print("\n🎉 所有测试完成")
