#!/usr/bin/env python3
"""
增强奖励系统测试脚本
===================

测试增强奖励系统的各个组件是否正常工作，
并提供详细的奖励分解分析。

作者: AI Assistant
日期: 2024年
"""

import sys
import os
import numpy as np

# 添加当前目录到Python路径
current_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(current_dir)

from gobigger_gym_env import GoBiggerEnv
from enhanced_reward_system import EnhancedRewardCalculator

def test_enhanced_reward_system():
    """测试增强奖励系统"""
    print("🧪 增强奖励系统测试")
    print("=" * 50)
    
    # 创建启用增强奖励的环境
    config = {
        'max_episode_steps': 100,
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
    
    print("✅ 创建环境（增强奖励模式）")
    env = GoBiggerEnv(config)
    
    print("✅ 重置环境")
    obs = env.reset()
    
    # 记录奖励历史
    reward_history = []
    component_history = []
    score_history = []
    
    print("✅ 开始测试步骤...")
    
    for step in range(20):  # 测试20步
        # 生成随机动作
        action = env.action_space.sample()
        
        # 记录步骤前的分数
        if env.current_obs and env.current_obs.player_states:
            score_before = list(env.current_obs.player_states.values())[0].score
        else:
            score_before = 0.0
        
        # 执行动作
        obs, reward, terminated, truncated, info = env.step(action)
        
        # 记录步骤后的分数
        if env.current_obs and env.current_obs.player_states:
            score_after = list(env.current_obs.player_states.values())[0].score
        else:
            score_after = 0.0
        
        score_delta = score_after - score_before
        
        # 记录数据
        reward_history.append(reward)
        score_history.append(score_after)
        
        # 获取奖励组件分解（如果可用）
        if hasattr(env, 'reward_components_history') and env.reward_components_history:
            latest_components = env.reward_components_history[-1]['components']
            component_history.append(latest_components)
        else:
            component_history.append({})
        
        # 显示步骤信息
        print(f"步骤 {step + 1:2d}: "
              f"动作={[f'{a:.2f}' for a in action]}, "
              f"分数={score_after:.2f} (Δ{score_delta:+.2f}), "
              f"奖励={reward:.4f}")
        
        # 如果有奖励组件，显示分解
        if component_history[-1]:
            print("         奖励组件:")
            for component, value in component_history[-1].items():
                weighted_value = config['enhanced_reward_weights'].get(component, 1.0) * value
                print(f"           {component}: {value:.4f} (权重后: {weighted_value:.4f})")
        
        # 如果episode结束，退出
        if terminated or truncated:
            print(f"🏁 Episode结束于步骤 {step + 1}")
            break
    
    print("\n📊 测试总结")
    print("-" * 50)
    
    if reward_history:
        total_reward = sum(reward_history)
        avg_reward = np.mean(reward_history)
        max_reward = max(reward_history)
        min_reward = min(reward_history)
        
        print(f"总步数: {len(reward_history)}")
        print(f"总奖励: {total_reward:.4f}")
        print(f"平均奖励: {avg_reward:.4f}")
        print(f"最大奖励: {max_reward:.4f}")
        print(f"最小奖励: {min_reward:.4f}")
        
        if score_history:
            initial_score = score_history[0] if len(score_history) > 1 else 0
            final_score = score_history[-1]
            score_growth = final_score - initial_score
            print(f"分数增长: {score_growth:+.2f} ({initial_score:.2f} → {final_score:.2f})")
        
        # 奖励组件统计
        if component_history and any(component_history):
            print("\n奖励组件平均值:")
            all_components = set()
            for components in component_history:
                all_components.update(components.keys())
            
            for component in sorted(all_components):
                values = [comp.get(component, 0.0) for comp in component_history if component in comp]
                if values:
                    avg_value = np.mean(values)
                    weighted_avg = avg_value * config['enhanced_reward_weights'].get(component, 1.0)
                    print(f"  {component}: {avg_value:.4f} (权重后: {weighted_avg:.4f})")
    
    env.close()
    print("\n✅ 测试完成！")

def test_reward_calculator_directly():
    """直接测试奖励计算器"""
    print("\n🔬 直接测试增强奖励计算器")
    print("=" * 50)
    
    calculator = EnhancedRewardCalculator()
    
    # 模拟一些游戏状态
    print("✅ 创建奖励计算器")
    print("✅ 模拟游戏状态...")
    
    # 这里可以添加更详细的单元测试
    # 由于需要真实的游戏状态对象，暂时跳过
    print("⚠️  需要真实游戏状态对象，跳过直接测试")
    
    # 显示奖励计算器配置
    print("\n奖励权重配置:")
    for component, weight in calculator.weights.items():
        print(f"  {component}: {weight}")

def compare_simple_vs_enhanced():
    """比较简单奖励和增强奖励"""
    print("\n⚖️  简单奖励 vs 增强奖励对比")
    print("=" * 50)
    
    results = {}
    
    for system_name, use_enhanced in [("简单奖励", False), ("增强奖励", True)]:
        print(f"\n🧪 测试{system_name}系统...")
        
        config = {
            'max_episode_steps': 50,
            'use_enhanced_reward': use_enhanced
        }
        
        env = GoBiggerEnv(config)
        
        # 运行5个episode
        all_rewards = []
        all_scores = []
        
        for episode in range(3):
            obs = env.reset()
            episode_reward = 0.0
            
            if env.current_obs and env.current_obs.player_states:
                initial_score = list(env.current_obs.player_states.values())[0].score
            else:
                initial_score = 0.0
            
            for step in range(20):  # 限制步数以加快测试
                action = env.action_space.sample()
                obs, reward, terminated, truncated, info = env.step(action)
                episode_reward += reward
                
                if terminated or truncated:
                    break
            
            final_score = info.get('final_score', initial_score)
            all_rewards.append(episode_reward)
            all_scores.append(final_score)
            
            print(f"  Episode {episode + 1}: 奖励={episode_reward:.4f}, 分数={final_score:.2f}")
        
        env.close()
        
        results[system_name] = {
            'avg_reward': np.mean(all_rewards),
            'avg_score': np.mean(all_scores),
            'reward_std': np.std(all_rewards),
            'score_std': np.std(all_scores)
        }
    
    print("\n📊 对比结果:")
    print(f"{'指标':<15} {'简单奖励':<15} {'增强奖励':<15} {'改进':<10}")
    print("-" * 60)
    
    simple = results["简单奖励"]
    enhanced = results["增强奖励"]
    
    metrics = [
        ('平均奖励', 'avg_reward'),
        ('平均分数', 'avg_score'),
        ('奖励标准差', 'reward_std'),
        ('分数标准差', 'score_std')
    ]
    
    for name, key in metrics:
        simple_val = simple[key]
        enhanced_val = enhanced[key]
        
        if simple_val != 0:
            if key in ['avg_reward', 'avg_score']:
                improvement = f"{((enhanced_val - simple_val) / abs(simple_val) * 100):+.1f}%"
            else:
                improvement = f"{((simple_val - enhanced_val) / abs(simple_val) * 100):+.1f}%"
        else:
            improvement = "N/A"
        
        print(f"{name:<15} {simple_val:<15.4f} {enhanced_val:<15.4f} {improvement:<10}")

def main():
    """主测试函数"""
    print("🧪 增强奖励系统完整测试套件")
    print("=" * 60)
    
    try:
        # 测试1: 增强奖励系统基本功能
        test_enhanced_reward_system()
        
        # 测试2: 直接测试奖励计算器
        test_reward_calculator_directly()
        
        # 测试3: 系统对比
        compare_simple_vs_enhanced()
        
        print("\n🎉 所有测试完成！")
        
    except Exception as e:
        print(f"\n❌ 测试过程中发生错误: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
