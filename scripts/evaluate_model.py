#!/usr/bin/env python3
"""
评估和测试训练好的强化学习模型
可视化智能体的行为和性能
"""
import sys
import os
from pathlib import Path
import numpy as np
import time
import matplotlib.pyplot as plt

# 路径设置
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

from gobigger_gym_env import GoBiggerEnv

try:
    from stable_baselines3 import PPO, DQN, A2C
    STABLE_BASELINES_AVAILABLE = True
except ImportError:
    STABLE_BASELINES_AVAILABLE = False
    print("❌ 需要 stable-baselines3 来加载训练好的模型")

def evaluate_model(model_path, episodes=10, render=True, save_replay=False):
    """详细评估训练好的模型"""
    print(f"🧪 评估模型: {model_path}")
    
    if not STABLE_BASELINES_AVAILABLE:
        print("❌ 需要 stable-baselines3")
        return
    
    # 加载模型
    try:
        if 'PPO' in model_path:
            model = PPO.load(model_path)
            print("✅ 加载PPO模型成功")
        elif 'DQN' in model_path:
            model = DQN.load(model_path)
            print("✅ 加载DQN模型成功")
        elif 'A2C' in model_path:
            model = A2C.load(model_path)
            print("✅ 加载A2C模型成功")
        else:
            print("❌ 无法识别模型类型，尝试用PPO加载...")
            model = PPO.load(model_path)
    except Exception as e:
        print(f"❌ 模型加载失败: {e}")
        return
    
    # 创建环境
    env = GoBiggerEnv({'max_episode_steps': 2000})
    
    episode_stats = []
    action_stats = {'directions': [], 'action_types': []}
    
    print(f"\n🎮 开始评估 {episodes} 个回合...")
    
    for episode in range(episodes):
        obs, info = env.reset()
        total_reward = 0
        steps = 0
        score_history = []
        actions_taken = []
        
        print(f"\n📝 回合 {episode + 1}:")
        
        while steps < 2000:
            # 预测动作
            action, _states = model.predict(obs, deterministic=True)
            actions_taken.append(action.copy())
            
            # 记录动作统计
            action_stats['directions'].append([action[0], action[1]])
            action_stats['action_types'].append(action[2])
            
            # 执行动作
            obs, reward, terminated, truncated, info = env.step(action)
            total_reward += reward
            steps += 1
            
            # 记录分数变化
            if hasattr(env, 'current_obs') and env.current_obs:
                if len(env.current_obs.player_states) > 0:
                    ps = list(env.current_obs.player_states.values())[0]
                    score_history.append(ps.score)
            
            # 可选渲染
            if render and steps % 100 == 0:
                print(f"  步骤 {steps}: 奖励={reward:.4f}, 动作=[{action[0]:.2f}, {action[1]:.2f}, {int(action[2])}]")
            
            if terminated or truncated:
                break
        
        # 统计本回合结果
        final_score = score_history[-1] if score_history else 0
        episode_stats.append({
            'episode': episode + 1,
            'total_reward': total_reward,
            'steps': steps,
            'final_score': final_score,
            'score_history': score_history,
            'actions': actions_taken
        })
        
        print(f"  ✅ 完成: 总奖励={total_reward:.3f}, 步数={steps}, 最终分数={final_score:.1f}")
    
    # 分析结果
    analyze_results(episode_stats, action_stats)
    
    # 可选保存回放数据
    if save_replay:
        save_replay_data(episode_stats, model_path)
    
    return episode_stats

def analyze_results(episode_stats, action_stats):
    """分析评估结果"""
    print("\n📊 评估结果分析:")
    print("=" * 50)
    
    # 基础统计
    rewards = [ep['total_reward'] for ep in episode_stats]
    steps = [ep['steps'] for ep in episode_stats]
    scores = [ep['final_score'] for ep in episode_stats]
    
    print(f"🎯 回合统计:")
    print(f"  平均奖励: {np.mean(rewards):.3f} ± {np.std(rewards):.3f}")
    print(f"  平均步数: {np.mean(steps):.1f} ± {np.std(steps):.1f}")
    print(f"  平均最终分数: {np.mean(scores):.1f} ± {np.std(scores):.1f}")
    print(f"  最佳奖励: {max(rewards):.3f}")
    print(f"  最高分数: {max(scores):.1f}")
    
    # 动作分析
    directions = np.array(action_stats['directions'])
    action_types = action_stats['action_types']
    
    print(f"\n🎮 动作分析:")
    print(f"  平均移动方向: ({np.mean(directions[:, 0]):.3f}, {np.mean(directions[:, 1]):.3f})")
    print(f"  移动幅度标准差: ({np.std(directions[:, 0]):.3f}, {np.std(directions[:, 1]):.3f})")
    
    # 动作类型分布
    action_counts = {0: 0, 1: 0, 2: 0}
    for act_type in action_types:
        action_counts[int(act_type)] += 1
    
    total_actions = len(action_types)
    print(f"  动作类型分布:")
    print(f"    无动作 (0): {action_counts[0]/total_actions*100:.1f}%")
    print(f"    吐球 (1): {action_counts[1]/total_actions*100:.1f}%") 
    print(f"    分裂 (2): {action_counts[2]/total_actions*100:.1f}%")
    
    # 绘制结果图表
    plot_evaluation_results(episode_stats, action_stats)

def plot_evaluation_results(episode_stats, action_stats):
    """绘制评估结果图表"""
    print("\n📈 生成结果图表...")
    
    plt.figure(figsize=(15, 10))
    
    # 1. 回合奖励趋势
    plt.subplot(2, 3, 1)
    rewards = [ep['total_reward'] for ep in episode_stats]
    plt.plot(rewards, 'bo-')
    plt.title('Episode Rewards')
    plt.xlabel('Episode')
    plt.ylabel('Total Reward')
    plt.grid(True)
    
    # 2. 回合步数
    plt.subplot(2, 3, 2)
    steps = [ep['steps'] for ep in episode_stats]
    plt.plot(steps, 'go-')
    plt.title('Episode Lengths')
    plt.xlabel('Episode')
    plt.ylabel('Steps')
    plt.grid(True)
    
    # 3. 分数变化（第一个回合）
    plt.subplot(2, 3, 3)
    if episode_stats and episode_stats[0]['score_history']:
        score_history = episode_stats[0]['score_history']
        plt.plot(score_history)
        plt.title('Score Evolution (Episode 1)')
        plt.xlabel('Steps')
        plt.ylabel('Score')
        plt.grid(True)
    
    # 4. 动作方向分布
    plt.subplot(2, 3, 4)
    directions = np.array(action_stats['directions'])
    plt.scatter(directions[:, 0], directions[:, 1], alpha=0.6)
    plt.title('Action Direction Distribution')
    plt.xlabel('Direction X')
    plt.ylabel('Direction Y')
    plt.grid(True)
    plt.axis('equal')
    
    # 5. 动作类型分布
    plt.subplot(2, 3, 5)
    action_types = action_stats['action_types']
    action_counts = [0, 0, 0]
    for act_type in action_types:
        action_counts[int(act_type)] += 1
    
    plt.bar(['No Action', 'Eject', 'Split'], action_counts)
    plt.title('Action Type Distribution')
    plt.ylabel('Count')
    
    # 6. 奖励直方图
    plt.subplot(2, 3, 6)
    plt.hist(rewards, bins=10, alpha=0.7)
    plt.title('Reward Distribution')
    plt.xlabel('Total Reward')
    plt.ylabel('Frequency')
    plt.grid(True)
    
    plt.tight_layout()
    plt.savefig('model_evaluation_results.png', dpi=150, bbox_inches='tight')
    plt.show()
    
    print("📊 结果图表已保存到 model_evaluation_results.png")

def save_replay_data(episode_stats, model_path):
    """保存回放数据"""
    import json
    
    replay_data = {
        'model_path': model_path,
        'timestamp': time.strftime('%Y-%m-%d_%H-%M-%S'),
        'episodes': episode_stats
    }
    
    # 转换numpy数组为列表以便JSON序列化
    for ep in replay_data['episodes']:
        if 'actions' in ep:
            ep['actions'] = [action.tolist() if hasattr(action, 'tolist') else action 
                           for action in ep['actions']]
    
    replay_file = f"replay_{time.strftime('%Y%m%d_%H%M%S')}.json"
    with open(replay_file, 'w') as f:
        json.dump(replay_data, f, indent=2)
    
    print(f"💾 回放数据已保存到 {replay_file}")

def compare_models(model_paths, episodes=5):
    """比较多个模型的性能"""
    print("🔍 模型性能比较")
    print("=" * 50)
    
    results = {}
    
    for model_path in model_paths:
        print(f"\n评估模型: {model_path}")
        stats = evaluate_model(model_path, episodes=episodes, render=False)
        
        if stats:
            rewards = [ep['total_reward'] for ep in stats]
            scores = [ep['final_score'] for ep in stats]
            
            results[model_path] = {
                'mean_reward': np.mean(rewards),
                'std_reward': np.std(rewards),
                'mean_score': np.mean(scores),
                'std_score': np.std(scores)
            }
    
    # 显示比较结果
    print("\n📊 模型比较结果:")
    print("-" * 80)
    print(f"{'模型路径':<30} {'平均奖励':<12} {'平均分数':<12} {'奖励稳定性':<12}")
    print("-" * 80)
    
    for model_path, result in results.items():
        model_name = Path(model_path).name
        print(f"{model_name:<30} {result['mean_reward']:<12.3f} {result['mean_score']:<12.1f} {result['std_reward']:<12.3f}")

def main():
    """主评估函数"""
    print("🧪 GoBigger 模型评估器")
    print("=" * 50)
    
    if not STABLE_BASELINES_AVAILABLE:
        print("❌ 需要安装 stable-baselines3 来评估模型")
        return
    
    print("🎯 选择评估模式:")
    print("1. 评估单个模型")
    print("2. 比较多个模型")
    print("3. 评估最新的PPO模型")
    
    choice = input("\n请选择 (1-3): ").strip()
    
    if choice == '1':
        model_path = input("请输入模型路径: ").strip()
        if os.path.exists(model_path):
            episodes = int(input("评估回合数 (默认10): ") or 10)
            evaluate_model(model_path, episodes=episodes, render=True, save_replay=True)
        else:
            print("❌ 模型文件不存在")
            
    elif choice == '2':
        print("请输入多个模型路径（每行一个，空行结束）:")
        model_paths = []
        while True:
            path = input().strip()
            if not path:
                break
            if os.path.exists(path):
                model_paths.append(path)
            else:
                print(f"⚠️  文件不存在: {path}")
        
        if model_paths:
            episodes = int(input("每个模型的评估回合数 (默认5): ") or 5)
            compare_models(model_paths, episodes=episodes)
        else:
            print("❌ 没有有效的模型路径")
            
    elif choice == '3':
        # 查找最新的PPO模型
        model_files = []
        
        # 检查models目录
        if os.path.exists("models"):
            for file in os.listdir("models"):
                if file.endswith(".zip") and "PPO" in file:
                    model_files.append(os.path.join("models", file))
        
        # 检查checkpoints目录
        if os.path.exists("checkpoints"):
            for file in os.listdir("checkpoints"):
                if file.endswith(".zip"):
                    model_files.append(os.path.join("checkpoints", file))
        
        if model_files:
            # 按修改时间排序，获取最新的
            latest_model = max(model_files, key=os.path.getmtime)
            print(f"🎯 找到最新模型: {latest_model}")
            
            episodes = int(input("评估回合数 (默认10): ") or 10)
            evaluate_model(latest_model, episodes=episodes, render=True, save_replay=True)
        else:
            print("❌ 未找到PPO模型文件")
    
    else:
        print("❌ 无效选择")

if __name__ == "__main__":
    main()
