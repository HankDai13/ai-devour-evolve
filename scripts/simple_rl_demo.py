#!/usr/bin/env python3
"""
快速强化学习训练演示
使用简化配置快速验证训练流程
"""
import sys
import os
from pathlib import Path
import numpy as np
import time

# 路径设置
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

from gobigger_gym_env import GoBiggerEnv

def simple_q_learning_demo():
    """简单的Q-learning演示"""
    print("🧠 简单Q-learning演示")
    
    # 创建环境
    env = GoBiggerEnv({'max_episode_steps': 200})
    
    # 简化状态空间：只使用前20个特征
    state_dim = 20
    action_dim = 3  # [direction_x, direction_y, action_type]
    
    # 简单的Q表（离散化状态和动作）
    q_table = {}
    
    # 超参数
    learning_rate = 0.1
    discount_factor = 0.95
    epsilon = 1.0  # 探索率
    epsilon_decay = 0.995
    epsilon_min = 0.01
    episodes = 50
    
    def discretize_state(obs):
        """将连续状态离散化"""
        # 只取前20个特征并量化到[-2, 2]范围内的10个离散值
        discrete_obs = np.clip(obs[:state_dim], -2, 2)
        discrete_obs = np.round(discrete_obs * 2).astype(int)  # 转换为整数
        return tuple(discrete_obs)
    
    def discretize_action(action_idx):
        """将动作索引转换为连续动作"""
        actions = [
            [0.0, 0.0, 0],      # 不动
            [1.0, 0.0, 0],      # 右移
            [-1.0, 0.0, 0],     # 左移
            [0.0, 1.0, 0],      # 上移
            [0.0, -1.0, 0],     # 下移
            [0.7, 0.7, 1],      # 右上吐球
            [0.0, 0.0, 2],      # 分裂
        ]
        return actions[action_idx % len(actions)]
    
    episode_rewards = []
    
    for episode in range(episodes):
        obs, info = env.reset()
        state = discretize_state(obs)
        total_reward = 0
        steps = 0
        
        while steps < 200:
            # ε-贪心策略选择动作
            if np.random.random() < epsilon:
                action_idx = np.random.randint(0, 7)  # 随机动作
            else:
                # 选择Q值最大的动作
                if state in q_table:
                    action_idx = np.argmax(q_table[state])
                else:
                    action_idx = np.random.randint(0, 7)
            
            # 执行动作
            action = discretize_action(action_idx)
            next_obs, reward, terminated, truncated, info = env.step(action)
            next_state = discretize_state(next_obs)
            
            # 更新Q表
            if state not in q_table:
                q_table[state] = np.zeros(7)
            
            if next_state not in q_table:
                q_table[next_state] = np.zeros(7)
            
            # Q-learning更新公式
            best_next_action = np.argmax(q_table[next_state])
            td_target = reward + discount_factor * q_table[next_state][best_next_action]
            td_error = td_target - q_table[state][action_idx]
            q_table[state][action_idx] += learning_rate * td_error
            
            state = next_state
            total_reward += reward
            steps += 1
            
            if terminated or truncated:
                break
        
        episode_rewards.append(total_reward)
        
        # 衰减探索率
        epsilon = max(epsilon_min, epsilon * epsilon_decay)
        
        if (episode + 1) % 10 == 0:
            avg_reward = np.mean(episode_rewards[-10:])
            print(f"Episode {episode + 1}: 平均奖励={avg_reward:.3f}, ε={epsilon:.3f}, Q表大小={len(q_table)}")
    
    print(f"\n📊 训练完成！")
    print(f"📈 最终平均奖励: {np.mean(episode_rewards[-10:]):.3f}")
    print(f"🧠 学习到的状态数: {len(q_table)}")
    
    return q_table, episode_rewards

def genetic_algorithm_demo():
    """简单的遗传算法演示"""
    print("🧬 遗传算法演示")
    
    env = GoBiggerEnv({'max_episode_steps': 100})
    
    # 神经网络参数（简化为线性模型）
    input_dim = 20  # 使用前20个特征
    hidden_dim = 10
    output_dim = 3  # [direction_x, direction_y, action_type]
    
    population_size = 20
    generations = 10
    mutation_rate = 0.1
    
    def create_individual():
        """创建一个个体（神经网络权重）"""
        w1 = np.random.randn(input_dim, hidden_dim) * 0.5
        b1 = np.random.randn(hidden_dim) * 0.1
        w2 = np.random.randn(hidden_dim, output_dim) * 0.5
        b2 = np.random.randn(output_dim) * 0.1
        return {'w1': w1, 'b1': b1, 'w2': w2, 'b2': b2}
    
    def forward(individual, state):
        """前向传播"""
        x = state[:input_dim]
        h = np.tanh(np.dot(x, individual['w1']) + individual['b1'])
        output = np.dot(h, individual['w2']) + individual['b2']
        
        # 输出转换为动作
        direction_x = np.tanh(output[0])  # [-1, 1]
        direction_y = np.tanh(output[1])  # [-1, 1]
        action_type = int(np.clip(output[2], 0, 2))  # [0, 1, 2]
        
        return [direction_x, direction_y, action_type]
    
    def evaluate_individual(individual, episodes=3):
        """评估个体性能"""
        total_rewards = []
        
        for _ in range(episodes):
            obs, info = env.reset()
            total_reward = 0
            steps = 0
            
            while steps < 100:
                action = forward(individual, obs)
                obs, reward, terminated, truncated, info = env.step(action)
                total_reward += reward
                steps += 1
                
                if terminated or truncated:
                    break
            
            total_rewards.append(total_reward)
        
        return np.mean(total_rewards)
    
    def crossover(parent1, parent2):
        """交叉操作"""
        child = {}
        for key in parent1.keys():
            mask = np.random.random(parent1[key].shape) > 0.5
            child[key] = np.where(mask, parent1[key], parent2[key])
        return child
    
    def mutate(individual):
        """变异操作"""
        mutated = {}
        for key in individual.keys():
            mutated[key] = individual[key].copy()
            mask = np.random.random(individual[key].shape) < mutation_rate
            mutated[key][mask] += np.random.randn(*individual[key].shape)[mask] * 0.1
        return mutated
    
    # 初始化种群
    population = [create_individual() for _ in range(population_size)]
    
    best_scores = []
    
    for generation in range(generations):
        # 评估所有个体
        scores = [evaluate_individual(ind) for ind in population]
        
        # 记录最佳性能
        best_score = max(scores)
        best_scores.append(best_score)
        avg_score = np.mean(scores)
        
        print(f"Generation {generation + 1}: 最佳={best_score:.3f}, 平均={avg_score:.3f}")
        
        # 选择（锦标赛选择）
        def tournament_selection():
            tournament_size = 3
            tournament = np.random.choice(len(population), tournament_size, replace=False)
            tournament_scores = [scores[i] for i in tournament]
            winner_idx = tournament[np.argmax(tournament_scores)]
            return population[winner_idx]
        
        # 生成新一代
        new_population = []
        
        # 保留最佳个体
        best_idx = np.argmax(scores)
        new_population.append(population[best_idx])
        
        # 交叉和变异
        while len(new_population) < population_size:
            parent1 = tournament_selection()
            parent2 = tournament_selection()
            child = crossover(parent1, parent2)
            child = mutate(child)
            new_population.append(child)
        
        population = new_population
    
    print(f"\n🏆 进化完成！最佳得分: {max(best_scores):.3f}")
    return population[np.argmax([evaluate_individual(ind) for ind in population])]

def main():
    """主演示函数"""
    print("🎯 选择演示算法:")
    print("1. Q-learning (基于表格)")
    print("2. 遗传算法 (神经进化)")
    print("3. 随机智能体对比")
    
    choice = input("\n请选择 (1-3): ").strip()
    
    if choice == '1':
        start_time = time.time()
        q_table, rewards = simple_q_learning_demo()
        print(f"⏱️  用时: {time.time() - start_time:.2f}秒")
        
    elif choice == '2':
        start_time = time.time()
        best_individual = genetic_algorithm_demo()
        print(f"⏱️  用时: {time.time() - start_time:.2f}秒")
        
    elif choice == '3':
        print("🎲 随机智能体基准测试")
        env = GoBiggerEnv({'max_episode_steps': 200})
        
        random_rewards = []
        for episode in range(20):
            obs, info = env.reset()
            total_reward = 0
            steps = 0
            
            while steps < 200:
                action = np.random.uniform(env.action_space_low, env.action_space_high)
                action[2] = int(action[2])
                obs, reward, terminated, truncated, info = env.step(action)
                total_reward += reward
                steps += 1
                
                if terminated or truncated:
                    break
            
            random_rewards.append(total_reward)
        
        print(f"🎲 随机策略平均奖励: {np.mean(random_rewards):.3f} ± {np.std(random_rewards):.3f}")
        
    else:
        print("❌ 无效选择")
    
    print("\n🎉 演示完成！")
    print("💡 这些简单算法展示了强化学习的基本原理")
    print("💡 更复杂的算法（如PPO、DQN）需要安装 stable-baselines3")

if __name__ == "__main__":
    main()
