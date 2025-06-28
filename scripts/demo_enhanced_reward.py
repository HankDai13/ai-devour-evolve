#!/usr/bin/env python3
"""
增强奖励系统成功案例演示
======================

通过更长的训练时间和更多的episode来展示增强奖励系统的效果。
此脚本会运行更长时间的测试，更有可能观察到分数增长。

作者: AI Assistant
日期: 2024年
"""

import sys
import os
import numpy as np
import time

# 添加当前目录到Python路径
current_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(current_dir)

from gobigger_gym_env import GoBiggerEnv

def run_extended_demo():
    """运行扩展演示，更有可能观察到分数增长"""
    print("🎮 增强奖励系统扩展演示")
    print("=" * 50)
    print("运行更长的episode以观察分数增长和奖励差异...")
    print("")
    
    # 配置：更长的episode，增强奖励
    config = {
        'max_episode_steps': 500,  # 更长的episode
        'use_enhanced_reward': True,
        'enhanced_reward_weights': {
            'score_growth': 3.0,        # 提高分数奖励权重以便观察
            'efficiency': 2.0,          
            'exploration': 1.0,         
            'strategic_split': 1.5,     
            'food_density': 1.2,        
            'survival': 0.01,           
            'time_penalty': -0.0005,    # 减少时间惩罚
            'death_penalty': -15.0,     
        }
    }
    
    env = GoBiggerEnv(config)
    
    successful_episodes = []
    total_episodes = 5
    
    for episode in range(total_episodes):
        print(f"\n📊 Episode {episode + 1}/{total_episodes}")
        print("-" * 30)
        
        obs = env.reset()
        episode_reward = 0.0
        step_count = 0
        reward_history = []
        score_history = []
        
        # 记录初始状态
        if env.current_obs and env.current_obs.player_states:
            initial_score = list(env.current_obs.player_states.values())[0].score
            max_score = initial_score
            score_increases = 0
        else:
            initial_score = 0.0
            max_score = 0.0
            score_increases = 0
        
        start_time = time.time()
        
        # 运行episode（更长时间）
        for step in range(500):  # 最多500步
            # 生成随机动作
            action = env.action_space.sample()
            
            # 记录步骤前分数
            if env.current_obs and env.current_obs.player_states:
                score_before = list(env.current_obs.player_states.values())[0].score
            else:
                score_before = initial_score
            
            # 执行动作
            obs, reward, terminated, truncated, info = env.step(action)
            episode_reward += reward
            step_count += 1
            
            # 记录步骤后分数
            if env.current_obs and env.current_obs.player_states:
                score_after = list(env.current_obs.player_states.values())[0].score
            else:
                score_after = score_before
            
            reward_history.append(reward)
            score_history.append(score_after)
            
            # 检查分数增长
            if score_after > score_before:
                score_increases += 1
                print(f"  步骤 {step + 1:3d}: 🎯 分数增长! "
                      f"{score_before:.1f} → {score_after:.1f} (奖励: {reward:+.4f})")
                
                # 显示奖励组件分解（如果可用）
                if hasattr(env, 'reward_components_history') and env.reward_components_history:
                    components = env.reward_components_history[-1]['components']
                    if components:
                        print(f"               奖励组件: ", end="")
                        for comp, val in components.items():
                            if abs(val) > 0.001:  # 只显示有意义的组件
                                weighted = val * config['enhanced_reward_weights'].get(comp, 1.0)
                                print(f"{comp}={weighted:+.3f} ", end="")
                        print()
            
            # 更新最高分数
            if score_after > max_score:
                max_score = score_after
            
            # 每50步显示一次进度
            if step > 0 and step % 50 == 0:
                current_score = score_after
                avg_reward = np.mean(reward_history[-50:])
                print(f"  步骤 {step:3d}: 当前分数={current_score:.1f}, "
                      f"平均奖励={avg_reward:+.4f}, 分数增长次数={score_increases}")
            
            # 检查是否结束
            if terminated or truncated:
                break
        
        # Episode总结
        duration = time.time() - start_time
        final_score = info.get('final_score', max_score)
        score_delta = final_score - initial_score
        avg_reward = np.mean(reward_history) if reward_history else 0.0
        
        print(f"\n  📋 Episode {episode + 1} 总结:")
        print(f"     初始分数: {initial_score:.1f}")
        print(f"     最终分数: {final_score:.1f}")
        print(f"     分数增长: {score_delta:+.1f}")
        print(f"     最高分数: {max_score:.1f}")
        print(f"     总奖励: {episode_reward:.4f}")
        print(f"     平均奖励: {avg_reward:.4f}")
        print(f"     步数: {step_count}")
        print(f"     分数增长次数: {score_increases}")
        print(f"     持续时间: {duration:.1f}秒")
        
        # 如果有分数增长，记录为成功案例
        if score_delta > 0:
            successful_episodes.append({
                'episode': episode + 1,
                'initial_score': initial_score,
                'final_score': final_score,
                'score_delta': score_delta,
                'total_reward': episode_reward,
                'avg_reward': avg_reward,
                'steps': step_count,
                'score_increases': score_increases,
                'duration': duration
            })
            print(f"     ✅ 成功案例！")
        else:
            print(f"     ⚠️  无分数增长")
    
    # 最终总结
    print(f"\n🎯 扩展演示总结")
    print("=" * 50)
    print(f"总Episodes: {total_episodes}")
    print(f"成功Episodes（有分数增长）: {len(successful_episodes)}")
    
    if successful_episodes:
        print(f"\n📊 成功案例统计:")
        total_score_delta = sum(ep['score_delta'] for ep in successful_episodes)
        avg_score_delta = total_score_delta / len(successful_episodes)
        max_score_delta = max(ep['score_delta'] for ep in successful_episodes)
        avg_reward = np.mean([ep['avg_reward'] for ep in successful_episodes])
        avg_steps = np.mean([ep['steps'] for ep in successful_episodes])
        avg_increases = np.mean([ep['score_increases'] for ep in successful_episodes])
        
        print(f"  平均分数增长: {avg_score_delta:.1f}")
        print(f"  最大分数增长: {max_score_delta:.1f}")
        print(f"  平均奖励: {avg_reward:.4f}")
        print(f"  平均步数: {avg_steps:.1f}")
        print(f"  平均分数增长次数: {avg_increases:.1f}")
        
        print(f"\n📈 最佳案例:")
        best_episode = max(successful_episodes, key=lambda x: x['score_delta'])
        print(f"  Episode {best_episode['episode']}: "
              f"分数增长 {best_episode['score_delta']:+.1f} "
              f"({best_episode['initial_score']:.1f} → {best_episode['final_score']:.1f})")
        print(f"  总奖励: {best_episode['total_reward']:.4f}")
        print(f"  分数增长次数: {best_episode['score_increases']}")
        
    else:
        print("⚠️  没有观察到分数增长的episode")
        print("   这可能是因为：")
        print("   1. 随机动作策略效率较低")
        print("   2. 需要更长的训练时间")
        print("   3. 可以尝试使用实际的RL算法而非随机动作")
    
    env.close()
    print(f"\n✅ 扩展演示完成！")

def compare_systems_extended():
    """扩展的系统对比，使用更长时间"""
    print(f"\n⚖️  扩展系统对比测试")
    print("=" * 50)
    
    configs = {
        '简单奖励': {
            'max_episode_steps': 200,
            'use_enhanced_reward': False
        },
        '增强奖励': {
            'max_episode_steps': 200,
            'use_enhanced_reward': True,
            'enhanced_reward_weights': {
                'score_growth': 2.0,
                'efficiency': 1.5,
                'exploration': 0.8,
                'strategic_split': 1.5,
                'food_density': 1.0,
                'survival': 0.01,
                'time_penalty': -0.0005,
                'death_penalty': -15.0,
            }
        }
    }
    
    results = {}
    
    for system_name, config in configs.items():
        print(f"\n🧪 测试 {system_name} 系统（扩展版）...")
        
        env = GoBiggerEnv(config)
        
        episode_rewards = []
        episode_scores = []
        score_deltas = []
        successful_episodes = 0
        
        # 运行更多episode
        for episode in range(5):
            obs = env.reset()
            episode_reward = 0.0
            
            if env.current_obs and env.current_obs.player_states:
                initial_score = list(env.current_obs.player_states.values())[0].score
            else:
                initial_score = 0.0
            
            # 运行更长时间
            for step in range(200):
                action = env.action_space.sample()
                obs, reward, terminated, truncated, info = env.step(action)
                episode_reward += reward
                
                if terminated or truncated:
                    break
            
            final_score = info.get('final_score', initial_score)
            score_delta = final_score - initial_score
            
            episode_rewards.append(episode_reward)
            episode_scores.append(final_score)
            score_deltas.append(score_delta)
            
            if score_delta > 0:
                successful_episodes += 1
            
            print(f"  Episode {episode + 1}: "
                  f"奖励={episode_reward:.4f}, "
                  f"分数={final_score:.1f} (Δ{score_delta:+.1f})")
        
        env.close()
        
        results[system_name] = {
            'avg_reward': np.mean(episode_rewards),
            'std_reward': np.std(episode_rewards),
            'avg_score': np.mean(episode_scores),
            'avg_score_delta': np.mean(score_deltas),
            'max_score_delta': max(score_deltas),
            'successful_episodes': successful_episodes,
            'success_rate': successful_episodes / len(episode_rewards) * 100
        }
    
    # 显示对比结果
    print(f"\n📊 扩展对比结果:")
    print(f"{'指标':<20} {'简单奖励':<15} {'增强奖励':<15} {'改进':<10}")
    print("-" * 70)
    
    simple = results['简单奖励']
    enhanced = results['增强奖励']
    
    metrics = [
        ('平均奖励', 'avg_reward'),
        ('奖励标准差', 'std_reward'),
        ('平均分数', 'avg_score'),
        ('平均分数增长', 'avg_score_delta'),
        ('最大分数增长', 'max_score_delta'),
        ('成功率(%)', 'success_rate')
    ]
    
    for name, key in metrics:
        simple_val = simple[key]
        enhanced_val = enhanced[key]
        
        if simple_val != 0:
            improvement = f"{((enhanced_val - simple_val) / abs(simple_val) * 100):+.1f}%"
        else:
            improvement = "N/A" if enhanced_val == 0 else "+∞"
        
        print(f"{name:<20} {simple_val:<15.3f} {enhanced_val:<15.3f} {improvement:<10}")
    
    # 结论
    print(f"\n🎯 测试结论:")
    if enhanced['success_rate'] > simple['success_rate']:
        print(f"✅ 增强奖励系统表现更好：")
        print(f"   - 成功率提高 {enhanced['success_rate'] - simple['success_rate']:.1f}%")
        print(f"   - 平均分数增长提高 {enhanced['avg_score_delta'] - simple['avg_score_delta']:+.1f}")
    elif enhanced['success_rate'] == simple['success_rate']:
        print(f"⚖️  两个系统表现相近，可能需要更长的测试时间")
    else:
        print(f"⚠️  简单奖励系统在此测试中表现更好")
        print(f"   这可能是由于随机动作和短时间测试的限制")

def main():
    """主函数"""
    print("🚀 增强奖励系统深度测试")
    print("=" * 60)
    
    try:
        # 扩展演示
        run_extended_demo()
        
        # 扩展对比
        compare_systems_extended()
        
        print(f"\n💡 建议:")
        print("1. 使用实际的RL算法（如PPO）而非随机动作进行训练")
        print("2. 增加训练时间和episode数量")
        print("3. 调整奖励权重以适应具体的训练目标")
        print("4. 监控奖励组件分解以优化策略")
        
    except KeyboardInterrupt:
        print(f"\n❌ 测试被用户中断")
    except Exception as e:
        print(f"\n❌ 测试过程中发生错误: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
