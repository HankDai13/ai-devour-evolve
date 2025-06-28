#!/usr/bin/env python3
"""
深入调试奖励传递问题
"""

import sys
import os
from pathlib import Path
import numpy as np

# 添加当前目录到路径
sys.path.insert(0, str(Path(__file__).parent))

from gobigger_gym_env import GoBiggerEnv

try:
    from stable_baselines3 import PPO
    from stable_baselines3.common.env_util import make_vec_env
    from stable_baselines3.common.vec_env import DummyVecEnv
    from stable_baselines3.common.monitor import Monitor
    from stable_baselines3.common.callbacks import BaseCallback
    SB3_AVAILABLE = True
except ImportError:
    SB3_AVAILABLE = False
    print("❌ 需要 stable-baselines3")

def create_test_env():
    """创建测试环境"""
    config = {
        'max_episode_steps': 50,  # 短episode用于快速测试
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
    env = GoBiggerEnv(config)
    return Monitor(env)

class DebugCallback(BaseCallback):
    def __init__(self):
        super().__init__()
        self.episode_count = 0
        
    def _on_step(self):
        # 检查每个step的info
        for i, info in enumerate(self.locals['infos']):
            if 'episode' in info:
                self.episode_count += 1
                episode_data = info['episode']
                print(f"\n📊 Episode {self.episode_count} 完成!")
                print(f"   奖励: {episode_data.get('r', 'N/A')}")
                print(f"   长度: {episode_data.get('l', 'N/A')}")
                print(f"   时间: {episode_data.get('t', 'N/A')}")
                
                # 检查其他info内容
                print(f"   完整info: {info}")
        
        # 检查logger内容
        if hasattr(self.model, 'logger') and hasattr(self.model.logger, 'name_to_value'):
            logger_data = self.model.logger.name_to_value
            episode_keys = [k for k in logger_data.keys() if 'ep_' in k or 'reward' in k.lower()]
            if episode_keys:
                print(f"\n🔍 Logger中的episode数据:")
                for key in episode_keys:
                    print(f"   {key}: {logger_data[key]}")
            else:
                if self.episode_count > 0:
                    print(f"\n⚠️  Logger中没有episode数据，但已完成{self.episode_count}个episode")
        
        return True

def test_monitor_and_sb3_integration():
    """测试Monitor与SB3的集成"""
    print("🧪 测试Monitor与SB3集成...")
    
    if not SB3_AVAILABLE:
        print("❌ 需要 stable-baselines3")
        return False
    
    # 创建向量化环境
    vec_env = make_vec_env(create_test_env, n_envs=1, vec_env_cls=DummyVecEnv)
    
    # 创建简单的PPO模型
    model = PPO(
        "MlpPolicy",
        vec_env,
        learning_rate=3e-4,
        n_steps=100,  # 小批次
        batch_size=32,
        n_epochs=2,
        verbose=1,
    )
    
    # 创建调试回调
    callback = DebugCallback()
    
    print("🚀 开始测试训练...")
    
    # 训练300步（应该足够完成几个episode）
    model.learn(total_timesteps=300, callback=callback)
    
    print("\n✅ 测试完成!")
    
    # 最终检查logger
    if hasattr(model, 'logger') and hasattr(model.logger, 'name_to_value'):
        logger_data = model.logger.name_to_value
        print(f"\n📋 最终Logger状态:")
        print(f"   总键数: {len(logger_data)}")
        
        episode_keys = [k for k in logger_data.keys() if 'ep_' in k or 'reward' in k.lower()]
        if episode_keys:
            print(f"   Episode相关键: {episode_keys}")
            for key in episode_keys:
                print(f"   {key}: {logger_data[key]}")
        else:
            print(f"   ⚠️  没有episode相关的键")
            print(f"   所有键: {list(logger_data.keys())}")
    
    vec_env.close()
    return True

if __name__ == "__main__":
    print("🔧 深入调试Monitor和SB3奖励传递")
    print("="*60)
    
    test_monitor_and_sb3_integration()
