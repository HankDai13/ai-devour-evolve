#!/usr/bin/env python3
"""
使用GoBigger核心引擎训练强化学习智能体
支持多种RL算法：PPO、DQN、A2C等
"""
import sys
import os
from pathlib import Path
import numpy as np
import time
import json
from collections import deque
import matplotlib.pyplot as plt

# 尝试导入rich库用于美化界面
try:
    from rich.console import Console
    from rich.table import Table
    from rich.progress import Progress, SpinnerColumn, TextColumn, BarColumn, TaskProgressColumn, TimeElapsedColumn, TimeRemainingColumn
    from rich.layout import Layout
    from rich.panel import Panel
    from rich.live import Live
    from rich.text import Text
    RICH_AVAILABLE = True
except ImportError:
    RICH_AVAILABLE = False
    print("💡 建议安装 rich 库获得更好的训练界面: pip install rich")

# 路径设置：定位到项目根目录
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

import gobigger_env
from gobigger_gym_env import GoBiggerEnv

try:
    # 尝试导入stable-baselines3 (如果已安装)
    from stable_baselines3 import PPO, DQN, A2C
    from stable_baselines3.common.env_util import make_vec_env
    from stable_baselines3.common.vec_env import DummyVecEnv
    from stable_baselines3.common.callbacks import EvalCallback, BaseCallback
    from stable_baselines3.common.logger import configure
    STABLE_BASELINES_AVAILABLE = True
    print("✅ 检测到 stable-baselines3，将使用专业RL算法")
except ImportError:
    STABLE_BASELINES_AVAILABLE = False
    print("⚠️  未检测到 stable-baselines3，将使用简单的随机策略演示")
    print("💡 安装命令: pip install stable-baselines3[extra]")

class TrainingCallback(BaseCallback):
    """训练过程监控回调（带Rich界面）"""
    
    def __init__(self, eval_freq=1000, save_freq=5000, verbose=1, total_timesteps=50000):
        super().__init__(verbose)
        self.eval_freq = eval_freq
        self.save_freq = save_freq
        self.best_mean_reward = -np.inf
        self.episode_rewards = deque(maxlen=100)
        self.episode_scores = deque(maxlen=100)
        self.total_timesteps = total_timesteps
        
        # Rich界面组件
        if RICH_AVAILABLE:
            self.console = Console()
            self.training_table = Table(title="🤖 GoBigger RL Training Status")
            self.training_table.add_column("Metric", style="cyan", no_wrap=True)
            self.training_table.add_column("Value", style="magenta")
            
            # 初始化训练统计
            self.training_stats = {
                "ep_len_mean": 0,
                "ep_rew_mean": 0,
                "ep_score_mean": 0,
                "fps": 0,
                "iterations": 0,
                "time_elapsed": 0,
                "total_timesteps": 0,
                "approx_kl": 0,
                "clip_fraction": 0,
                "entropy_loss": 0,
                "learning_rate": 0,
                "loss": 0,
                "policy_gradient_loss": 0,
                "value_loss": 0,
                "episodes_completed": 0
            }
            
            self.start_time = time.time()
            self.last_table_update = 0
            self.table_update_interval = 5  # 每5秒更新一次表格
        
    def _update_training_table(self):
        """更新训练状态表格"""
        if not RICH_AVAILABLE:
            return
            
        # 清除旧表格内容
        self.training_table = Table(title="🤖 GoBigger RL Training Status")
        self.training_table.add_column("Category", style="cyan", no_wrap=True)
        self.training_table.add_column("Metric", style="blue", no_wrap=True)
        self.training_table.add_column("Value", style="magenta")
        
        # Rollout metrics
        self.training_table.add_row("rollout/", "ep_len_mean", f"{self.training_stats['ep_len_mean']:.0f}")
        self.training_table.add_row("", "ep_rew_mean", f"{self.training_stats['ep_rew_mean']:.2f}")
        self.training_table.add_row("", "ep_score_mean", f"{self.training_stats['ep_score_mean']:.0f}")
        
        # Time metrics
        self.training_table.add_row("time/", "fps", f"{self.training_stats['fps']:.0f}")
        self.training_table.add_row("", "iterations", f"{self.training_stats['iterations']}")
        self.training_table.add_row("", "time_elapsed", f"{self.training_stats['time_elapsed']:.0f}")
        self.training_table.add_row("", "total_timesteps", f"{self.training_stats['total_timesteps']}")
        
        # Training metrics
        if self.training_stats['approx_kl'] > 0:
            self.training_table.add_row("train/", "approx_kl", f"{self.training_stats['approx_kl']:.6f}")
            self.training_table.add_row("", "clip_fraction", f"{self.training_stats['clip_fraction']:.4f}")
            self.training_table.add_row("", "entropy_loss", f"{self.training_stats['entropy_loss']:.2f}")
            self.training_table.add_row("", "learning_rate", f"{self.training_stats['learning_rate']:.6f}")
            self.training_table.add_row("", "loss", f"{self.training_stats['loss']:.6f}")
            self.training_table.add_row("", "policy_gradient_loss", f"{self.training_stats['policy_gradient_loss']:.6f}")
            self.training_table.add_row("", "value_loss", f"{self.training_stats['value_loss']:.6f}")
        
        # Episode info
        self.training_table.add_row("episodes/", "completed", f"{self.training_stats['episodes_completed']}")
        
    def _on_step(self) -> bool:
        # 收集奖励和分数统计
        for info in self.locals['infos']:
            if 'final_score' in info:
                final_score = info['final_score']
                score_delta = info.get('score_delta', 0)
                episode_length = info.get('episode_length', 0)
                
                self.episode_scores.append(final_score)
                self.training_stats['episodes_completed'] += 1
                
                if not RICH_AVAILABLE and self.verbose > 0:
                    print(f"🎯 Episode 结束 - 最终分数: {final_score:.2f}, "
                          f"分数变化: {score_delta:+.2f}, 步数: {episode_length}")
            
            if 'episode' in info:
                episode_reward = info['episode']['r']
                self.episode_rewards.append(episode_reward)
        
        # 更新训练统计
        if RICH_AVAILABLE:
            current_time = time.time()
            self.training_stats['time_elapsed'] = current_time - self.start_time
            self.training_stats['total_timesteps'] = self.num_timesteps
            
            # 从logger获取训练指标
            if hasattr(self.model, 'logger') and self.model.logger.name_to_value:
                logger_data = self.model.logger.name_to_value
                
                # Rollout metrics
                if 'rollout/ep_len_mean' in logger_data:
                    self.training_stats['ep_len_mean'] = logger_data['rollout/ep_len_mean']
                if 'rollout/ep_rew_mean' in logger_data:
                    self.training_stats['ep_rew_mean'] = logger_data['rollout/ep_rew_mean']
                
                # Time metrics
                if 'time/fps' in logger_data:
                    self.training_stats['fps'] = logger_data['time/fps']
                if 'time/iterations' in logger_data:
                    self.training_stats['iterations'] = logger_data['time/iterations']
                
                # Training metrics
                if 'train/approx_kl' in logger_data:
                    self.training_stats['approx_kl'] = logger_data['train/approx_kl']
                if 'train/clip_fraction' in logger_data:
                    self.training_stats['clip_fraction'] = logger_data['train/clip_fraction']
                if 'train/entropy_loss' in logger_data:
                    self.training_stats['entropy_loss'] = logger_data['train/entropy_loss']
                if 'train/learning_rate' in logger_data:
                    self.training_stats['learning_rate'] = logger_data['train/learning_rate']
                if 'train/loss' in logger_data:
                    self.training_stats['loss'] = logger_data['train/loss']
                if 'train/policy_gradient_loss' in logger_data:
                    self.training_stats['policy_gradient_loss'] = logger_data['train/policy_gradient_loss']
                if 'train/value_loss' in logger_data:
                    self.training_stats['value_loss'] = logger_data['train/value_loss']
            
            # 更新平均分数
            if len(self.episode_scores) > 0:
                self.training_stats['ep_score_mean'] = np.mean(self.episode_scores)
        
        # 定期评估和保存（简化版，避免干扰界面）
        if self.num_timesteps % self.eval_freq == 0:
            if len(self.episode_rewards) > 0:
                mean_reward = np.mean(self.episode_rewards)
                mean_score = np.mean(self.episode_scores) if len(self.episode_scores) > 0 else 0
                
                if mean_reward > self.best_mean_reward:
                    self.best_mean_reward = mean_reward
                    if not RICH_AVAILABLE and self.verbose > 0:
                        print(f"🎉 新的最佳模型! 平均奖励: {mean_reward:.2f}")
        
        # 定期保存检查点
        if self.num_timesteps % self.save_freq == 0:
            model_path = f"checkpoints/model_{self.num_timesteps}_steps.zip"
            self.model.save(model_path)
            if not RICH_AVAILABLE and self.verbose > 0:
                print(f"💾 保存模型检查点: {model_path}")
        
        return True

def create_env(config=None):
    """创建训练环境"""
    default_config = {
        'max_episode_steps': 2000,  # 每局最大步数
    }
    if config:
        default_config.update(config)
    
    return GoBiggerEnv(default_config)

def train_with_stable_baselines3(algorithm='PPO', total_timesteps=100000, config=None):
    """使用stable-baselines3训练智能体（带Rich界面）"""
    print(f"🚀 开始使用 {algorithm} 算法训练...")
    
    # 创建环境
    env = make_vec_env(lambda: create_env(config), n_envs=1, vec_env_cls=DummyVecEnv)
    
    # 创建模型
    if algorithm == 'PPO':
        model = PPO(
            "MlpPolicy", 
            env,
            learning_rate=3e-4,
            n_steps=2048,
            batch_size=64,
            n_epochs=10,
            gamma=0.99,
            gae_lambda=0.95,
            clip_range=0.2,
            verbose=0 if RICH_AVAILABLE else 1,  # Rich界面时静默模式
            tensorboard_log="./tensorboard_logs/"
        )
    elif algorithm == 'DQN':
        model = DQN(
            "MlpPolicy",
            env,
            learning_rate=1e-4,
            buffer_size=50000,
            learning_starts=1000,
            batch_size=32,
            tau=1.0,
            gamma=0.99,
            train_freq=4,
            gradient_steps=1,
            target_update_interval=1000,
            verbose=0 if RICH_AVAILABLE else 1,
            tensorboard_log="./tensorboard_logs/"
        )
    elif algorithm == 'A2C':
        model = A2C(
            "MlpPolicy",
            env,
            learning_rate=7e-4,
            n_steps=5,
            gamma=0.99,
            gae_lambda=1.0,
            ent_coef=0.01,
            vf_coef=0.25,
            max_grad_norm=0.5,
            verbose=0 if RICH_AVAILABLE else 1,
            tensorboard_log="./tensorboard_logs/"
        )
    else:
        raise ValueError(f"不支持的算法: {algorithm}")
    
    # 创建回调
    os.makedirs("checkpoints", exist_ok=True)
    
    if RICH_AVAILABLE:
        # 创建Rich界面版本的回调
        class RichTrainingCallback(TrainingCallback):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)
                
            def _on_step(self):
                # 更新训练统计
                current_time = time.time()
                self.training_stats['time_elapsed'] = current_time - self.start_time
                self.training_stats['total_timesteps'] = self.num_timesteps
                
                # 从模型获取训练指标
                if hasattr(self.model, 'logger') and hasattr(self.model.logger, 'name_to_value'):
                    logger_data = self.model.logger.name_to_value
                    
                    # 更新各项指标
                    for key, value in logger_data.items():
                        if key in ['rollout/ep_len_mean', 'rollout/ep_rew_mean', 'time/fps', 
                                  'time/iterations', 'train/approx_kl', 'train/clip_fraction',
                                  'train/entropy_loss', 'train/learning_rate', 'train/loss',
                                  'train/policy_gradient_loss', 'train/value_loss']:
                            stat_name = key.split('/')[-1]
                            if stat_name in self.training_stats:
                                self.training_stats[stat_name] = value
                
                # 更新平均分数
                if len(self.episode_scores) > 0:
                    self.training_stats['ep_score_mean'] = np.mean(self.episode_scores)
                
                # 定期更新显示
                if current_time - self.last_table_update > self.table_update_interval:
                    self._update_training_table()
                    self.last_table_update = current_time
                    
                    # 显示当前进度
                    progress_percent = (self.num_timesteps / self.total_timesteps) * 100
                    self.console.clear()
                    self.console.print(self.training_table)
                    self.console.print(f"\n📈 训练进度: {progress_percent:.1f}% "
                                     f"({self.num_timesteps:,}/{self.total_timesteps:,} steps)")
                    
                    if len(self.episode_scores) > 0:
                        latest_score = self.episode_scores[-1] if self.episode_scores else 0
                        self.console.print(f"🎯 最新Episode分数: {latest_score:.0f}")
                
                return super()._on_step()
        
        callback = RichTrainingCallback(eval_freq=2000, save_freq=10000, total_timesteps=total_timesteps)
        
        # 开始训练
        print(f"📈 开始训练，目标步数: {total_timesteps}")
        print("💡 训练界面将定期更新...")
        start_time = time.time()
        
        model.learn(
            total_timesteps=total_timesteps,
            callback=callback,
            tb_log_name=f"{algorithm}_gobigger"
        )
        
        train_time = time.time() - start_time
        callback.console.print(f"\n✅ 训练完成！用时: {train_time:.2f}秒", style="green bold")
        
    else:
        # 传统文本界面训练
        callback = TrainingCallback(eval_freq=2000, save_freq=10000, total_timesteps=total_timesteps)
        
        print(f"📈 开始训练，目标步数: {total_timesteps}")
        start_time = time.time()
        
        model.learn(
            total_timesteps=total_timesteps,
            callback=callback,
            tb_log_name=f"{algorithm}_gobigger"
        )
        
        train_time = time.time() - start_time
        print(f"✅ 训练完成！用时: {train_time:.2f}秒")
    
    # 保存最终模型
    final_model_path = f"models/{algorithm}_gobigger_final.zip"
    os.makedirs("models", exist_ok=True)
    model.save(final_model_path)
    print(f"💾 最终模型已保存: {final_model_path}")
    
    return model

def simple_random_training(episodes=100):
    """简单的随机策略演示（当没有stable-baselines3时）"""
    print("🎮 运行随机策略演示训练...")
    
    env = create_env({'max_episode_steps': 500})
    
    episode_rewards = []
    episode_lengths = []
    
    for episode in range(episodes):
        obs, info = env.reset()
        total_reward = 0
        steps = 0
        
        while True:
            # 随机动作
            action = np.random.uniform(env.action_space_low, env.action_space_high)
            action[2] = int(action[2])  # 动作类型为整数
            
            obs, reward, terminated, truncated, info = env.step(action)
            total_reward += reward
            steps += 1
            
            if terminated or truncated:
                # 显示episode结束信息
                if 'final_score' in info:
                    final_score = info['final_score']
                    score_delta = info.get('score_delta', 0)
                    if (episode + 1) % 10 == 0:  # 每10个episode显示一次详细信息
                        print(f"  Episode {episode + 1} 结束 - 最终分数: {final_score:.2f}, "
                              f"分数变化: {score_delta:+.2f}, 总奖励: {total_reward:.3f}")
                break
        
        episode_rewards.append(total_reward)
        episode_lengths.append(steps)
        
        if (episode + 1) % 10 == 0:
            avg_reward = np.mean(episode_rewards[-10:])
            avg_length = np.mean(episode_lengths[-10:])
            print(f"Episode {episode + 1}: 平均奖励={avg_reward:.3f}, 平均长度={avg_length:.1f}")
    
    # 绘制训练曲线
    plt.figure(figsize=(12, 4))
    
    plt.subplot(1, 2, 1)
    plt.plot(episode_rewards)
    plt.title('Episode Rewards')
    plt.xlabel('Episode')
    plt.ylabel('Total Reward')
    
    plt.subplot(1, 2, 2)
    plt.plot(episode_lengths)
    plt.title('Episode Lengths')
    plt.xlabel('Episode')
    plt.ylabel('Steps')
    
    plt.tight_layout()
    plt.savefig('random_training_results.png')
    plt.show()
    
    print(f"📊 训练结果已保存到 random_training_results.png")
    print(f"📈 最终平均奖励: {np.mean(episode_rewards[-20:]):.3f}")

def evaluate_model(model_path, episodes=10):
    """评估训练好的模型"""
    if not STABLE_BASELINES_AVAILABLE:
        print("❌ 需要 stable-baselines3 来加载和评估模型")
        return
    
    print(f"🧪 评估模型: {model_path}")
    
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
    
    # 创建环境
    env = create_env({'max_episode_steps': 1000})
    
    episode_rewards = []
    episode_lengths = []
    
    for episode in range(episodes):
        obs, info = env.reset()
        total_reward = 0
        steps = 0
        
        while True:
            action, _states = model.predict(obs, deterministic=True)
            obs, reward, terminated, truncated, info = env.step(action)
            total_reward += reward
            steps += 1
            
            if terminated or truncated:
                # 显示episode结束信息
                if 'final_score' in info:
                    final_score = info['final_score']
                    score_delta = info.get('score_delta', 0)
                    print(f"Episode {episode + 1}: 奖励={total_reward:.3f}, 步数={steps}, "
                          f"最终分数={final_score:.2f}, 分数变化={score_delta:+.2f}")
                else:
                    print(f"Episode {episode + 1}: 奖励={total_reward:.3f}, 步数={steps}")
                break
        
        episode_rewards.append(total_reward)
        episode_lengths.append(steps)
    
    print(f"📊 评估结果:")
    print(f"  平均奖励: {np.mean(episode_rewards):.3f} ± {np.std(episode_rewards):.3f}")
    print(f"  平均步数: {np.mean(episode_lengths):.1f} ± {np.std(episode_lengths):.1f}")

def main():
    """主训练函数"""
    print("🤖 GoBigger 强化学习训练器")
    print("=" * 50)
    
    if not RICH_AVAILABLE:
        print("💡 建议安装 rich 库获得更好的训练界面: pip install rich")
        print("   当前使用传统文本界面")
        print()
    else:
        console = Console()
        console.print("✨ 检测到 Rich 库，使用美化界面", style="green bold")
        print()
    
    # 训练配置
    config = {
        'max_episode_steps': 1500,  # 每局最大步数
    }
    
    if STABLE_BASELINES_AVAILABLE:
        print("🎯 选择训练算法:")
        print("1. PPO (推荐) - Proximal Policy Optimization")
        print("2. DQN - Deep Q-Network") 
        print("3. A2C - Advantage Actor-Critic")
        print("4. 评估现有模型")
        print("5. 随机策略演示")
        
        choice = input("\n请选择 (1-5): ").strip()
        
        if choice == '1':
            model = train_with_stable_baselines3('PPO', total_timesteps=50000, config=config)
        elif choice == '2':
            model = train_with_stable_baselines3('DQN', total_timesteps=50000, config=config)
        elif choice == '3':
            model = train_with_stable_baselines3('A2C', total_timesteps=50000, config=config)
        elif choice == '4':
            model_path = input("请输入模型路径: ").strip()
            if os.path.exists(model_path):
                evaluate_model(model_path)
            else:
                print("❌ 模型文件不存在")
        elif choice == '5':
            simple_random_training(episodes=50)
        else:
            print("❌ 无效选择")
            
    else:
        simple_random_training(episodes=50)
    
    print("\n🎉 训练完成！")
    print("💡 提示：")
    print("  - 使用 tensorboard --logdir ./tensorboard_logs 查看训练曲线")
    print("  - 模型保存在 ./models/ 目录")
    print("  - 检查点保存在 ./checkpoints/ 目录")
    if not RICH_AVAILABLE:
        print("  - 建议安装 rich 库享受更好的训练界面体验: pip install rich")

if __name__ == "__main__":
    main()
