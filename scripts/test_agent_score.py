#!/usr/bin/env python3
"""
快速测试智能体分数表现
专门用于观察智能体是否能吃到食物
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

from gobigger_gym_env import GoBiggerEnv

def test_random_agent(episodes=10):
    """测试随机智能体的分数表现"""
    print("🎮 测试随机智能体分数表现")
    print("=" * 50)
    
    env = GoBiggerEnv({'max_episode_steps': 1000})
    
    scores = []
    score_deltas = []
    
    for episode in range(episodes):
        obs, info = env.reset()
        total_reward = 0
        steps = 0
        
        print(f"\n🚀 Episode {episode + 1} 开始...")
        
        while True:
            # 随机动作
            action = np.random.uniform(env.action_space_low, env.action_space_high)
            action[2] = int(action[2])
            
            obs, reward, terminated, truncated, info = env.step(action)
            total_reward += reward
            steps += 1
            
            if terminated or truncated:
                if 'final_score' in info:
                    final_score = info['final_score']
                    score_delta = info.get('score_delta', 0)
                    episode_length = info.get('episode_length', steps)
                    
                    scores.append(final_score)
                    score_deltas.append(score_delta)
                    
                    print(f"✅ Episode {episode + 1} 结束:")
                    print(f"   📊 最终分数: {final_score:.2f}")
                    print(f"   📈 分数变化: {score_delta:+.2f}")
                    print(f"   🕒 步数: {episode_length}")
                    print(f"   🎯 总奖励: {total_reward:.3f}")
                    
                    if score_delta > 0:
                        print(f"   🎉 成功吃到食物!")
                    else:
                        print(f"   😢 没有吃到食物")
                else:
                    print(f"   ❌ 无法获取分数信息")
                break
    
    # 总结统计
    print("\n" + "=" * 50)
    print("📊 测试总结:")
    if scores:
        print(f"   平均最终分数: {np.mean(scores):.2f}")
        print(f"   平均分数变化: {np.mean(score_deltas):+.2f}")
        print(f"   最高分数: {max(scores):.2f}")
        print(f"   最低分数: {min(scores):.2f}")
        
        success_count = sum(1 for delta in score_deltas if delta > 0)
        print(f"   成功吃到食物的轮次: {success_count}/{len(score_deltas)}")
        print(f"   成功率: {success_count/len(score_deltas)*100:.1f}%")
    else:
        print("   ❌ 未获取到有效分数数据")

def test_trained_agent(model_path, episodes=5):
    """测试训练好的智能体"""
    try:
        from stable_baselines3 import PPO, DQN, A2C
        
        print(f"🤖 测试训练好的智能体: {model_path}")
        print("=" * 50)
        
        # 加载模型
        if 'PPO' in model_path:
            model = PPO.load(model_path)
        elif 'DQN' in model_path:
            model = DQN.load(model_path)
        elif 'A2C' in model_path:
            model = A2C.load(model_path)
        else:
            print("❌ 无法识别模型类型")
            return
        
        env = GoBiggerEnv({'max_episode_steps': 1000})
        
        scores = []
        score_deltas = []
        
        for episode in range(episodes):
            obs, info = env.reset()
            total_reward = 0
            steps = 0
            
            print(f"\n🚀 Episode {episode + 1} 开始...")
            
            while True:
                action, _states = model.predict(obs, deterministic=True)
                obs, reward, terminated, truncated, info = env.step(action)
                total_reward += reward
                steps += 1
                
                if terminated or truncated:
                    if 'final_score' in info:
                        final_score = info['final_score']
                        score_delta = info.get('score_delta', 0)
                        episode_length = info.get('episode_length', steps)
                        
                        scores.append(final_score)
                        score_deltas.append(score_delta)
                        
                        print(f"✅ Episode {episode + 1} 结束:")
                        print(f"   📊 最终分数: {final_score:.2f}")
                        print(f"   📈 分数变化: {score_delta:+.2f}")
                        print(f"   🕒 步数: {episode_length}")
                        print(f"   🎯 总奖励: {total_reward:.3f}")
                        
                        if score_delta > 0:
                            print(f"   🎉 成功吃到食物!")
                        else:
                            print(f"   😢 没有吃到食物")
                    break
        
        # 总结统计
        print("\n" + "=" * 50)
        print("📊 训练智能体测试总结:")
        if scores:
            print(f"   平均最终分数: {np.mean(scores):.2f}")
            print(f"   平均分数变化: {np.mean(score_deltas):+.2f}")
            print(f"   最高分数: {max(scores):.2f}")
            print(f"   最低分数: {min(scores):.2f}")
            
            success_count = sum(1 for delta in score_deltas if delta > 0)
            print(f"   成功吃到食物的轮次: {success_count}/{len(score_deltas)}")
            print(f"   成功率: {success_count/len(score_deltas)*100:.1f}%")
        
    except ImportError:
        print("❌ 需要 stable-baselines3 来测试训练好的模型")

def main():
    print("🧪 GoBigger 智能体分数测试器")
    print("用于快速观察智能体是否能吃到食物")
    print("=" * 60)
    
    print("🎯 选择测试模式:")
    print("1. 测试随机智能体 (10轮)")
    print("2. 测试训练好的模型")
    print("3. 快速测试随机智能体 (3轮)")
    
    try:
        choice = input("请选择 (1-3): ").strip()
        
        if choice == "1":
            test_random_agent(episodes=10)
        elif choice == "2":
            # 查找可用的模型
            models_dir = Path("models")
            checkpoints_dir = Path("checkpoints")
            
            model_files = []
            if models_dir.exists():
                model_files.extend(list(models_dir.glob("*.zip")))
            if checkpoints_dir.exists():
                model_files.extend(list(checkpoints_dir.glob("*.zip")))
            
            if model_files:
                print("📁 可用模型:")
                for i, model_file in enumerate(model_files):
                    print(f"   {i+1}. {model_file}")
                
                model_choice = input("选择模型编号: ").strip()
                try:
                    model_idx = int(model_choice) - 1
                    if 0 <= model_idx < len(model_files):
                        test_trained_agent(str(model_files[model_idx]))
                    else:
                        print("❌ 无效的模型编号")
                except ValueError:
                    print("❌ 请输入有效的数字")
            else:
                print("❌ 未找到训练好的模型")
                print("💡 请先运行 train_rl_agent.py 训练模型")
        elif choice == "3":
            test_random_agent(episodes=3)
        else:
            print("❌ 无效选择")
            
    except KeyboardInterrupt:
        print("\n👋 测试已取消")
    except Exception as e:
        print(f"❌ 测试过程中出错: {e}")

if __name__ == "__main__":
    main()
