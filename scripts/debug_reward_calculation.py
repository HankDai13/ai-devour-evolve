#!/usr/bin/env python3
"""
调试奖励计算的详细脚本
分析每一步的奖励计算过程，找出为什么奖励一直为0
"""
import sys
import os
from pathlib import Path
import numpy as np

# 添加路径
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
if build_dir.exists():
    sys.path.insert(0, str(build_dir))
    os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

from gobigger_gym_env import GoBiggerEnv

def debug_reward_step_by_step():
    """逐步调试奖励计算"""
    print("🔍 开始逐步调试奖励计算...")
    
    # 创建环境
    env = GoBiggerEnv({'max_episode_steps': 50})
    obs, info = env.reset()
    
    print(f"初始观察维度: {obs.shape}")
    print(f"初始信息: {info}")
    
    total_reward = 0
    rewards_detail = []
    
    for step in range(20):  # 运行20步进行详细分析
        # 选择随机动作
        action = env.action_space.sample()
        
        # 记录步骤前的状态
        if env.current_obs and env.current_obs.player_states:
            ps_before = list(env.current_obs.player_states.values())[0]
            score_before = ps_before.score
            cells_before = len(ps_before.clone) if hasattr(ps_before, 'clone') and isinstance(ps_before.clone, list) else 1
            can_eject_before = ps_before.can_eject
            can_split_before = ps_before.can_split
        else:
            score_before = 0
            cells_before = 0
            can_eject_before = False
            can_split_before = False
        
        # 执行动作
        obs, reward, terminated, truncated, info = env.step(action)
        
        # 记录步骤后的状态
        if env.current_obs and env.current_obs.player_states:
            ps_after = list(env.current_obs.player_states.values())[0]
            score_after = ps_after.score
            cells_after = len(ps_after.clone) if hasattr(ps_after, 'clone') and isinstance(ps_after.clone, list) else 1
            can_eject_after = ps_after.can_eject
            can_split_after = ps_after.can_split
        else:
            score_after = 0
            cells_after = 0
            can_eject_after = False
            can_split_after = False
        
        # 手动计算奖励组件（模拟奖励函数逻辑）
        score_delta = score_after - score_before
        score_reward = score_delta / 100.0
        time_penalty = -0.001
        death_penalty = -10.0 if env.engine.is_done() else 0.0
        survival_reward = 0.01 if not env.engine.is_done() else 0.0
        cell_delta = cells_after - cells_before
        size_reward = cell_delta * 0.1
        
        manual_reward = score_reward + time_penalty + death_penalty + survival_reward + size_reward
        
        # 记录详细信息
        step_detail = {
            'step': step + 1,
            'action': action.tolist() if hasattr(action, 'tolist') else action,
            'score_before': score_before,
            'score_after': score_after,
            'score_delta': score_delta,
            'cells_before': cells_before,
            'cells_after': cells_after,
            'cell_delta': cell_delta,
            'can_eject_before': can_eject_before,
            'can_eject_after': can_eject_after,
            'can_split_before': can_split_before,
            'can_split_after': can_split_after,
            'score_reward': score_reward,
            'time_penalty': time_penalty,
            'death_penalty': death_penalty,
            'survival_reward': survival_reward,
            'size_reward': size_reward,
            'manual_reward': manual_reward,
            'env_reward': reward,
            'reward_match': abs(manual_reward - reward) < 1e-6,
            'terminated': terminated,
            'truncated': truncated,
            'info': info
        }
        
        rewards_detail.append(step_detail)
        total_reward += reward
        
        # 打印步骤详情
        print(f"\n--- 步骤 {step + 1} ---")
        print(f"动作: {step_detail['action']}")
        print(f"分数变化: {score_before:.2f} → {score_after:.2f} (Δ{score_delta:+.2f})")
        print(f"细胞数变化: {cells_before} → {cells_after} (Δ{cell_delta:+d})")
        print(f"能力状态: 吐球({can_eject_before}→{can_eject_after}), 分裂({can_split_before}→{can_split_after})")
        print(f"奖励组件:")
        print(f"  - 分数奖励: {score_reward:+.4f}")
        print(f"  - 时间惩罚: {time_penalty:+.4f}")
        print(f"  - 死亡惩罚: {death_penalty:+.4f}")
        print(f"  - 生存奖励: {survival_reward:+.4f}")
        print(f"  - 尺寸奖励: {size_reward:+.4f}")
        print(f"手动计算奖励: {manual_reward:+.4f}")
        print(f"环境返回奖励: {reward:+.4f}")
        print(f"奖励匹配: {'✅' if step_detail['reward_match'] else '❌'}")
        print(f"游戏状态: 终止={terminated}, 截断={truncated}")
        
        if terminated or truncated:
            print(f"🏁 Episode 结束在步骤 {step + 1}")
            break
    
    # 总结
    print("\n" + "="*60)
    print("📊 奖励分析总结")
    print("="*60)
    
    non_zero_rewards = [r for r in rewards_detail if abs(r['env_reward']) > 1e-6]
    positive_rewards = [r for r in rewards_detail if r['env_reward'] > 0]
    negative_rewards = [r for r in rewards_detail if r['env_reward'] < 0]
    
    print(f"总步数: {len(rewards_detail)}")
    print(f"非零奖励步数: {len(non_zero_rewards)}")
    print(f"正奖励步数: {len(positive_rewards)}")
    print(f"负奖励步数: {len(negative_rewards)}")
    print(f"总奖励: {total_reward:.4f}")
    print(f"平均奖励: {total_reward / len(rewards_detail):.4f}")
    
    # 分析奖励组件
    total_score_reward = sum(r['score_reward'] for r in rewards_detail)
    total_time_penalty = sum(r['time_penalty'] for r in rewards_detail)
    total_death_penalty = sum(r['death_penalty'] for r in rewards_detail)
    total_survival_reward = sum(r['survival_reward'] for r in rewards_detail)
    total_size_reward = sum(r['size_reward'] for r in rewards_detail)
    
    print(f"\n奖励组件总和:")
    print(f"  - 分数奖励总和: {total_score_reward:+.4f}")
    print(f"  - 时间惩罚总和: {total_time_penalty:+.4f}")
    print(f"  - 死亡惩罚总和: {total_death_penalty:+.4f}")
    print(f"  - 生存奖励总和: {total_survival_reward:+.4f}")
    print(f"  - 尺寸奖励总和: {total_size_reward:+.4f}")
    
    # 找出分数有变化的步骤
    score_change_steps = [r for r in rewards_detail if abs(r['score_delta']) > 1e-6]
    if score_change_steps:
        print(f"\n📈 分数有变化的步骤 ({len(score_change_steps)} 个):")
        for r in score_change_steps:
            print(f"  步骤 {r['step']}: 分数 {r['score_before']:.2f} → {r['score_after']:.2f} "
                  f"(Δ{r['score_delta']:+.2f}), 奖励: {r['env_reward']:+.4f}")
    else:
        print(f"\n⚠️ 没有步骤的分数发生变化！")
    
    # 检查奖励计算一致性
    mismatched_rewards = [r for r in rewards_detail if not r['reward_match']]
    if mismatched_rewards:
        print(f"\n❌ 奖励计算不一致的步骤 ({len(mismatched_rewards)} 个):")
        for r in mismatched_rewards:
            print(f"  步骤 {r['step']}: 手动={r['manual_reward']:+.4f}, "
                  f"环境={r['env_reward']:+.4f}, 差值={r['manual_reward']-r['env_reward']:+.4f}")
    else:
        print(f"\n✅ 所有步骤的奖励计算都一致")
    
    return rewards_detail

def check_reward_function_implementation():
    """检查奖励函数的具体实现"""
    print("\n🔍 检查奖励函数实现...")
    
    env = GoBiggerEnv({'max_episode_steps': 10})
    obs, info = env.reset()
    
    # 检查初始状态
    if env.current_obs and env.current_obs.player_states:
        ps = list(env.current_obs.player_states.values())[0]
        print(f"初始分数: {ps.score}")
        print(f"last_score: {env.last_score}")
        print(f"initial_score: {env.initial_score}")
        
        # 检查clone属性
        if hasattr(ps, 'clone'):
            print(f"clone 属性类型: {type(ps.clone)}")
            print(f"clone 内容: {ps.clone}")
            if isinstance(ps.clone, list):
                print(f"clone 列表长度: {len(ps.clone)}")
            else:
                print(f"clone 不是列表")
        else:
            print(f"❌ player_state 没有 clone 属性")
        
        # 测试手动奖励计算
        print(f"\n测试手动奖励计算:")
        manual_reward = env._calculate_reward()
        print(f"手动计算的奖励: {manual_reward}")
    
    # 执行一步
    action = env.action_space.sample()
    obs, reward, terminated, truncated, info = env.step(action)
    
    print(f"\n执行一步后:")
    print(f"动作: {action}")
    print(f"奖励: {reward}")
    print(f"terminated: {terminated}")
    print(f"truncated: {truncated}")
    
    if env.current_obs and env.current_obs.player_states:
        ps = list(env.current_obs.player_states.values())[0]
        print(f"新分数: {ps.score}")
        print(f"last_score: {env.last_score}")

if __name__ == "__main__":
    print("🚀 开始奖励计算调试")
    
    # 第一部分：检查奖励函数实现
    check_reward_function_implementation()
    
    # 第二部分：逐步调试
    debug_reward_step_by_step()
    
    print("\n🎯 调试完成！")
