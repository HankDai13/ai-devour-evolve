#!/usr/bin/env python3
"""
测试Monitor修复是否解决了奖励显示问题
"""

import sys
import os
from pathlib import Path

# 添加当前目录到路径
sys.path.insert(0, str(Path(__file__).parent))

from gobigger_gym_env import GoBiggerEnv

try:
    from stable_baselines3.common.monitor import Monitor
    from stable_baselines3.common.env_util import make_vec_env
    from stable_baselines3.common.vec_env import DummyVecEnv
    SB3_AVAILABLE = True
except ImportError:
    SB3_AVAILABLE = False
    print("⚠️ stable-baselines3 不可用，仅测试基础环境")

def test_monitor_reward_tracking():
    """测试Monitor是否正确跟踪奖励"""
    print("🧪 测试Monitor奖励跟踪...")
    
    if not SB3_AVAILABLE:
        print("❌ 需要 stable-baselines3")
        return False
    
    # 创建标准环境
    config = {
        'max_episode_steps': 50,  # 较短的episode用于快速测试
        'use_enhanced_reward': False
    }
    
    env = GoBiggerEnv(config)
    env = Monitor(env)  # 添加Monitor
    
    print("🔄 运行测试episode...")
    
    obs = env.reset()
    total_reward = 0
    episode_rewards = []
    
    for step in range(50):
        action = env.action_space.sample()
        obs, reward, terminated, truncated, info = env.step(action)
        done = terminated or truncated
        total_reward += reward
        
        print(f"步骤 {step+1}: 奖励={reward:.4f}, 累计奖励={total_reward:.4f}")
        
        if done:
            print(f"\n✅ Episode结束:")
            print(f"   总奖励: {total_reward:.4f}")
            print(f"   Info: {info}")
            
            # 检查Monitor是否记录了episode统计
            if 'episode' in info:
                episode_data = info['episode']
                print(f"   Monitor记录的episode奖励: {episode_data.get('r', 'N/A')}")
                print(f"   Monitor记录的episode长度: {episode_data.get('l', 'N/A')}")
                return True
            else:
                print("   ❌ Monitor没有记录episode统计")
                return False
            
    print("❌ Episode没有自然结束")
    return False

def test_enhanced_reward_monitor():
    """测试增强奖励系统与Monitor的配合"""
    print("\n🧪 测试增强奖励+Monitor...")
    
    if not SB3_AVAILABLE:
        print("❌ 需要 stable-baselines3")
        return False
    
    # 创建增强奖励环境
    enhanced_config = {
        'max_episode_steps': 50,
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
    
    env = GoBiggerEnv(enhanced_config)
    env = Monitor(env)  # 添加Monitor
    
    print("🔄 运行增强奖励测试episode...")
    
    obs = env.reset()
    total_reward = 0
    
    for step in range(50):
        action = env.action_space.sample()
        obs, reward, terminated, truncated, info = env.step(action)
        done = terminated or truncated
        total_reward += reward
        
        if step % 10 == 0:
            print(f"步骤 {step+1}: 奖励={reward:.4f}, 累计奖励={total_reward:.4f}")
        
        if done:
            print(f"\n✅ 增强奖励Episode结束:")
            print(f"   总奖励: {total_reward:.4f}")
            print(f"   Info: {info}")
            
            # 检查Monitor是否记录了episode统计
            if 'episode' in info:
                episode_data = info['episode']
                print(f"   Monitor记录的episode奖励: {episode_data.get('r', 'N/A')}")
                print(f"   Monitor记录的episode长度: {episode_data.get('l', 'N/A')}")
                return True
            else:
                print("   ❌ Monitor没有记录episode统计")
                return False
            
    print("❌ Episode没有自然结束")
    return False

def test_vectorized_env():
    """测试向量化环境是否正确传递Monitor统计"""
    print("\n🧪 测试向量化环境+Monitor...")
    
    if not SB3_AVAILABLE:
        print("❌ 需要 stable-baselines3")
        return False
    
    def create_monitored_env():
        config = {
            'max_episode_steps': 30,
            'use_enhanced_reward': True
        }
        env = GoBiggerEnv(config)
        return Monitor(env)
    
    # 创建向量化环境
    vec_env = make_vec_env(create_monitored_env, n_envs=1, vec_env_cls=DummyVecEnv)
    
    print("🔄 运行向量化环境测试...")
    
    obs = vec_env.reset()
    total_rewards = [0]
    
    for step in range(30):
        actions = [vec_env.action_space.sample()]
        obs, rewards, dones, infos = vec_env.step(actions)
        total_rewards[0] += rewards[0]
        
        if step % 10 == 0:
            print(f"步骤 {step+1}: 奖励={rewards[0]:.4f}, 累计奖励={total_rewards[0]:.4f}")
        
        if dones[0]:
            print(f"\n✅ 向量化环境Episode结束:")
            print(f"   总奖励: {total_rewards[0]:.4f}")
            print(f"   Info: {infos[0]}")
            
            # 检查是否有episode统计
            if 'episode' in infos[0]:
                episode_data = infos[0]['episode']
                print(f"   Monitor记录的episode奖励: {episode_data.get('r', 'N/A')}")
                print(f"   Monitor记录的episode长度: {episode_data.get('l', 'N/A')}")
                vec_env.close()
                return True
            else:
                print("   ❌ 向量化环境没有传递episode统计")
                vec_env.close()
                return False
    
    print("❌ Episode没有自然结束")
    vec_env.close()
    return False

if __name__ == "__main__":
    print("🔧 测试Monitor修复效果")
    print("="*50)
    
    # 测试基础Monitor功能
    result1 = test_monitor_reward_tracking()
    
    # 测试增强奖励
    result2 = test_enhanced_reward_monitor()
    
    # 测试向量化环境
    result3 = test_vectorized_env()
    
    print("\n" + "="*50)
    print("📊 测试结果总结:")
    print(f"   基础Monitor: {'✅ 通过' if result1 else '❌ 失败'}")
    print(f"   增强奖励Monitor: {'✅ 通过' if result2 else '❌ 失败'}")
    print(f"   向量化环境Monitor: {'✅ 通过' if result3 else '❌ 失败'}")
    
    if all([result1, result2, result3]):
        print("\n🎉 所有测试通过！Monitor修复成功！")
    else:
        print("\n⚠️ 部分测试失败，需要进一步调试")
