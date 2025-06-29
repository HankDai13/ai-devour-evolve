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
    from rich.align import Align
    from rich.columns import Columns
    RICH_AVAILABLE = True
except ImportError:
    RICH_AVAILABLE = False
    print("💡 建议安装 rich 库获得更好的训练界面: pip install rich")

def convert_numpy_types(obj):
    """递归转换numpy类型为原生Python类型，解决JSON序列化问题"""
    if isinstance(obj, np.integer):
        return int(obj)
    elif isinstance(obj, np.floating):
        return float(obj)
    elif isinstance(obj, np.ndarray):
        return obj.tolist()
    elif isinstance(obj, dict):
        return {key: convert_numpy_types(value) for key, value in obj.items()}
    elif isinstance(obj, list):
        return [convert_numpy_types(item) for item in obj]
    elif isinstance(obj, tuple):
        return tuple(convert_numpy_types(item) for item in obj)
    else:
        return obj

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
    from stable_baselines3.common.monitor import Monitor
    STABLE_BASELINES_AVAILABLE = True
    print("✅ 检测到 stable-baselines3，将使用专业RL算法")
except ImportError:
    STABLE_BASELINES_AVAILABLE = False
    print("⚠️  未检测到 stable-baselines3，将使用简单的随机策略演示")
    print("💡 安装命令: pip install stable-baselines3[extra]")

class TrainingCallback(BaseCallback):
    """训练过程监控回调（支持增强奖励显示和长时间训练优化）"""
    
    def __init__(self, eval_freq=1000, save_freq=5000, verbose=1, total_timesteps=50000, 
                 auto_save_freq=10000, checkpoint_freq=25000, enable_lr_decay=False):
        super().__init__(verbose)
        self.eval_freq = eval_freq
        self.save_freq = save_freq
        self.auto_save_freq = auto_save_freq  # 自动保存频率
        self.checkpoint_freq = checkpoint_freq  # 检查点保存频率
        self.enable_lr_decay = enable_lr_decay  # 学习率衰减
        self.best_mean_reward = -np.inf
        self.episode_rewards = deque(maxlen=100)
        self.episode_scores = deque(maxlen=100)
        self.total_timesteps = total_timesteps
        self.use_enhanced_reward = False  # 默认值，会在外部设置
        
        # 长时间训练相关
        self.initial_lr = None  # 初始学习率，在训练开始时设置
        self.last_auto_save = 0
        self.last_checkpoint = 0
        
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
            
            # 创建进度条
            self.progress = Progress(
                SpinnerColumn(),
                TextColumn("[progress.description]{task.description}"),
                BarColumn(),
                TaskProgressColumn(),
                TimeElapsedColumn(),
                TimeRemainingColumn(),
                console=self.console,
                expand=True
            )
            self.progress_task = None
        
    def _update_training_table(self):
        """更新训练状态表格"""
        if not RICH_AVAILABLE:
            return
            
        # 清除旧表格内容
        reward_type = "增强奖励" if self.use_enhanced_reward else "标准奖励"
        self.training_table = Table(title=f"🤖 GoBigger RL Training Status ({reward_type})")
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
        
        # 增强奖励系统特有信息
        if self.use_enhanced_reward:
            self.training_table.add_row("", "", "")  # 分隔线
            self.training_table.add_row("enhanced/", "system", "🎯 密集奖励信号")
            self.training_table.add_row("", "components", "多维度奖励")
        
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
            
            # 手动收集episode reward信息
            if 'episode' in info:
                episode_reward = info['episode']['r']
                episode_len = info['episode']['l']
                self.episode_rewards.append(episode_reward)
                
                # 手动更新episode统计
                if len(self.episode_rewards) > 0:
                    self.training_stats['ep_rew_mean'] = np.mean(self.episode_rewards)
                if len(self.episode_rewards) > 0:
                    # 使用episode长度信息
                    ep_lengths = [info['episode']['l'] for info in self.locals.get('infos', []) if 'episode' in info]
                    if ep_lengths:
                        self.training_stats['ep_len_mean'] = np.mean(ep_lengths)
                    else:
                        self.training_stats['ep_len_mean'] = episode_len
        
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
        
        # 🚀 长时间训练增强功能
        # 自动保存（更频繁，防止意外丢失）
        if self.num_timesteps - self.last_auto_save >= self.auto_save_freq:
            auto_save_path = f"models/auto_save_{self.num_timesteps}.zip"
            self.model.save(auto_save_path)
            self.last_auto_save = self.num_timesteps
            if not RICH_AVAILABLE and self.verbose > 0:
                print(f"💾 自动保存: {auto_save_path}")
        
        # 检查点保存（用于长时间训练恢复）
        if self.num_timesteps - self.last_checkpoint >= self.checkpoint_freq:
            checkpoint_path = f"checkpoints/checkpoint_{self.num_timesteps}.zip"
            self.model.save(checkpoint_path)
            self.last_checkpoint = self.num_timesteps
            
            # 保存训练统计（转换numpy类型以避免JSON序列化错误）
            stats_path = f"checkpoints/stats_{self.num_timesteps}.json"
            stats_data = {
                'timesteps': self.num_timesteps,
                'best_reward': self.best_mean_reward,
                'episode_rewards': list(self.episode_rewards),
                'episode_scores': list(self.episode_scores),
                'training_stats': self.training_stats,
                'elapsed_hours': (time.time() - self.start_time) / 3600
            }
            # 转换所有numpy类型为原生Python类型
            stats_data = convert_numpy_types(stats_data)
            
            with open(stats_path, 'w') as f:
                json.dump(stats_data, f, indent=2)
            
            if not RICH_AVAILABLE and self.verbose > 0:
                elapsed_hours = (time.time() - self.start_time) / 3600
                print(f"📋 检查点保存: {checkpoint_path} (训练 {elapsed_hours:.1f} 小时)")
        
        # 学习率衰减（长时间训练优化）
        if self.enable_lr_decay and self.initial_lr is not None:
            progress = self.num_timesteps / self.total_timesteps
            # 线性衰减到初始学习率的10%
            new_lr = self.initial_lr * (1.0 - 0.9 * progress)
            if hasattr(self.model, 'lr_schedule'):
                # 更新学习率
                if hasattr(self.model.policy.optimizer, 'param_groups'):
                    for param_group in self.model.policy.optimizer.param_groups:
                        param_group['lr'] = new_lr
            if not RICH_AVAILABLE and self.verbose > 0:
                print(f"💾 保存模型检查点: {model_path}")
        
        return True

def create_env(config=None):
    """创建训练环境（带Monitor）"""
    default_config = {
        'max_episode_steps': 2000,  # 每局最大步数
        'use_enhanced_reward': False,  # 默认使用简单奖励
        'debug_rewards': False,  # 🔥 新增：奖励调试选项
    }
    if config:
        default_config.update(config)
    
    # 创建环境并用Monitor包装（重要：这是episode统计的关键）
    env = GoBiggerEnv(default_config)
    
    # 🔥 启用奖励调试（如果请求）
    if default_config.get('debug_rewards', False):
        env.debug_rewards = True
        print("🔍 启用奖励调试模式 - 将显示Split/Eject动作奖励")
    
    if STABLE_BASELINES_AVAILABLE:
        env = Monitor(env)
    return env

def create_enhanced_env(config=None):
    """创建增强奖励训练环境（带Monitor）"""
    enhanced_config = {
        'max_episode_steps': 2000,
        'use_enhanced_reward': True,  # 启用增强奖励系统
        'enhanced_reward_weights': {
            'score_growth': 2.0,        # 分数增长奖励权重
            'efficiency': 1.5,          # 效率奖励权重
            'exploration': 0.8,         # 探索奖励权重
            'strategic_split': 2.0,     # 战略分裂奖励权重
            'food_density': 1.0,        # 食物密度奖励权重
            'survival': 0.02,           # 生存奖励权重
            'time_penalty': -0.001,     # 时间惩罚权重
            'death_penalty': -20.0,     # 死亡惩罚权重
        }
    }
    if config:
        enhanced_config.update(config)
    
    # 创建环境并用Monitor包装（重要：这是episode统计的关键）
    env = GoBiggerEnv(enhanced_config)
    if STABLE_BASELINES_AVAILABLE:
        env = Monitor(env)
    return env

def train_with_stable_baselines3(algorithm='PPO', total_timesteps=100000, config=None, use_enhanced_reward=False):
    """使用stable-baselines3训练智能体（支持增强奖励系统）"""
    reward_type = "增强奖励" if use_enhanced_reward else "标准奖励"
    print(f"🚀 开始使用 {algorithm} 算法训练 ({reward_type})...")
    
    # 创建环境 - 使用vectorized environment以正确支持episode统计
    def make_env():
        if use_enhanced_reward:
            return create_enhanced_env(config)
        else:
            return create_env(config)
    
    if use_enhanced_reward:
        print("✨ 使用增强奖励系统 - 提供更密集的奖励信号")
    else:
        print("📊 使用标准奖励系统")
    
    # 使用make_vec_env来正确集成Monitor和episode统计
    env = make_vec_env(make_env, n_envs=1, vec_env_cls=DummyVecEnv)
    
    # 🚀 长时间训练优化的网络配置
    def get_optimized_policy_kwargs(total_timesteps):
        """根据训练规模优化网络结构"""
        if total_timesteps >= 3000000:  # 超超长时间训练（400万步级别）
            return dict(
                net_arch=[512, 512, 256, 128],  # 更深更宽的网络
                activation_fn=torch.nn.ReLU,
                share_features_extractor=False
            )
        elif total_timesteps >= 1000000:  # 超长时间训练
            return dict(
                net_arch=[256, 256, 128],  # 更深的网络
                activation_fn=torch.nn.ReLU,
                share_features_extractor=False
            )
        elif total_timesteps >= 500000:  # 长时间训练
            return dict(
                net_arch=[128, 128],
                activation_fn=torch.nn.ReLU
            )
        else:  # 短时间训练
            return dict(
                net_arch=[64, 64],
                activation_fn=torch.nn.ReLU
            )
    
    try:
        import torch
        policy_kwargs = get_optimized_policy_kwargs(total_timesteps)
        print(f"🧠 网络结构: {policy_kwargs['net_arch']} (适配 {total_timesteps:,} 步训练)")
    except ImportError:
        policy_kwargs = None
        print("⚠️  未检测到PyTorch，使用默认网络结构")
    
    # 🎯 长时间训练优化的超参数
    def get_optimized_hyperparams(algorithm, total_timesteps):
        """根据算法和训练规模优化超参数"""
        if algorithm == 'PPO':
            if total_timesteps >= 3000000:  # 超超长时间训练（400万步级别）
                return {
                    "learning_rate": 2e-4,  # 更低的学习率，更稳定
                    "n_steps": 2048,  # 更大的rollout，更好的采样效率
                    "batch_size": 256,  # 更大的批量，更稳定的梯度
                    "n_epochs": 6,  # 🔥 减少epochs防止过拟合！(Gemini建议4-10)
                    "gamma": 0.998,  # 更高的折扣因子，考虑长期回报
                    "gae_lambda": 0.99,  # 更高的GAE参数
                    "clip_range": 0.15,  # 稍大的裁剪范围，增加学习灵活性
                    "ent_coef": 0.015,  # 🔥 增加熵系数鼓励探索！(Gemini建议0.01-0.02)
                    "vf_coef": 0.25,  # 价值函数系数
                    "max_grad_norm": 0.5  # 稍微放松梯度裁剪
                }
            elif total_timesteps >= 1000000:  # 超长时间训练
                return {
                    "learning_rate": 2.5e-4,  # 稍低的学习率
                    "n_steps": 1024,  # 更大的rollout
                    "batch_size": 128,  # 更大的批量
                    "n_epochs": 8,  # 🔥 减少epochs防止过拟合！(Gemini建议4-10)
                    "gamma": 0.995,  # 更高的折扣因子
                    "gae_lambda": 0.98,  # GAE参数
                    "clip_range": 0.18,  # 稍大的裁剪范围
                    "ent_coef": 0.012,  # 🔥 增加熵系数鼓励探索！(Gemini建议0.01-0.02)
                    "vf_coef": 0.5,  # 价值函数系数
                    "max_grad_norm": 0.5  # 梯度裁剪
                }
            elif total_timesteps >= 500000:  # 长时间训练
                return {
                    "learning_rate": 3e-4,
                    "n_steps": 512,
                    "batch_size": 64,
                    "n_epochs": 6,  # 🔥 减少epochs防止过拟合！
                    "gamma": 0.99,
                    "gae_lambda": 0.95,
                    "clip_range": 0.2,
                    "ent_coef": 0.01  # 🔥 增加熵系数鼓励探索！
                }
            else:  # 标准训练
                return {
                    "learning_rate": 3e-4,
                    "n_steps": 256,
                    "batch_size": 32,
                    "n_epochs": 5,  # 🔥 减少epochs防止过拟合！
                    "gamma": 0.99,
                    "gae_lambda": 0.95,
                    "clip_range": 0.2,
                    "ent_coef": 0.01  # 🔥 增加熵系数鼓励探索！
                }
        return {}
    
    hyperparams = get_optimized_hyperparams(algorithm, total_timesteps)
    
    # 创建模型
    if algorithm == 'PPO':
        model = PPO(
            "MlpPolicy", 
            env,
            policy_kwargs=policy_kwargs,
            verbose=0 if RICH_AVAILABLE else 1,  # Rich界面时静默模式
            tensorboard_log="./tensorboard_logs/",
            **hyperparams
        )
    elif algorithm == 'DQN':
        model = DQN(
            "MlpPolicy",
            env,
            policy_kwargs=policy_kwargs,
            learning_rate=1e-4,
            buffer_size=min(100000, max(50000, total_timesteps // 10)),  # 自适应缓冲区
            learning_starts=max(1000, total_timesteps // 100),
            batch_size=64 if total_timesteps >= 100000 else 32,
            tau=1.0,
            gamma=0.99,
            train_freq=4,
            gradient_steps=1,
            target_update_interval=max(500, total_timesteps // 100),
            verbose=0 if RICH_AVAILABLE else 1,
            tensorboard_log="./tensorboard_logs/"
        )
    elif algorithm == 'A2C':
        model = A2C(
            "MlpPolicy",
            env,
            policy_kwargs=policy_kwargs,
            learning_rate=7e-4,
            n_steps=8 if total_timesteps >= 100000 else 5,
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
    
    # 🚀 创建长时间训练优化的回调
    os.makedirs("checkpoints", exist_ok=True)
    os.makedirs("models", exist_ok=True)
    
    # 动态调整回调频率
    if total_timesteps >= 3000000:  # 超超长时间训练（400万步级别）
        eval_freq = 10000
        save_freq = 50000
        auto_save_freq = 25000
        checkpoint_freq = 100000
        enable_lr_decay = True
        print(f"🌟 超长时间训练模式: 评估间隔={eval_freq}, 保存间隔={save_freq}, 自动保存={auto_save_freq}")
        print(f"   预计训练时间: 20-30小时 (适合周末长时间训练)")
    elif total_timesteps >= 1000000:  # 超长时间训练
        eval_freq = 5000
        save_freq = 25000
        auto_save_freq = 10000
        checkpoint_freq = 50000
        enable_lr_decay = True
        print(f"🎯 长时间训练模式: 评估间隔={eval_freq}, 保存间隔={save_freq}, 自动保存={auto_save_freq}")
    elif total_timesteps >= 500000:  # 长时间训练
        eval_freq = 2500
        save_freq = 10000
        auto_save_freq = 5000
        checkpoint_freq = 25000
        enable_lr_decay = True
        print(f"🎯 中长训练模式: 评估间隔={eval_freq}, 保存间隔={save_freq}")
    else:  # 标准训练
        eval_freq = 1000
        save_freq = 5000
        auto_save_freq = 2500
        checkpoint_freq = 10000
        enable_lr_decay = False
    
    if RICH_AVAILABLE:
        # 创建Rich界面版本的回调
        class RichTrainingCallback(TrainingCallback):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.use_enhanced_reward = use_enhanced_reward  # 在父类初始化后设置
                
                # 设置初始学习率（用于学习率衰减）
                if hasattr(model, 'learning_rate'):
                    if callable(model.learning_rate):
                        self.initial_lr = model.learning_rate(1.0)  # 获取初始学习率
                    else:
                        self.initial_lr = model.learning_rate
                    print(f"📊 初始学习率: {self.initial_lr}")
                
                # print(f"🔍 回调初始化调试: use_enhanced_reward = {self.use_enhanced_reward}")
                
                # 初始化进度条任务
                if hasattr(self, 'progress'):
                    reward_type = "增强奖励" if self.use_enhanced_reward else "标准奖励"
                    if total_timesteps >= 3000000:
                        training_mode = "超长时间"
                    elif total_timesteps >= 1000000:
                        training_mode = "长时间"
                    elif total_timesteps >= 500000:
                        training_mode = "中长时间"
                    else:
                        training_mode = "标准"
                    self.progress_task = self.progress.add_task(
                        f"[green]🚀 {algorithm} {training_mode}训练 ({reward_type})", 
                        total=total_timesteps
                    )
                
            def _on_step(self):
                # 更新训练统计
                current_time = time.time()
                self.training_stats['time_elapsed'] = current_time - self.start_time
                self.training_stats['total_timesteps'] = self.num_timesteps
                
                # 从模型获取训练指标
                if hasattr(self.model, 'logger') and hasattr(self.model.logger, 'name_to_value'):
                    logger_data = self.model.logger.name_to_value
                    
                    # 调试：打印所有logger数据 (可选，取消注释以启用调试)
                    # if self.num_timesteps % 1000 == 0:  # 每1000步打印一次调试信息
                    #     print(f"\n🔍 Logger完整数据调试 (步数: {self.num_timesteps}):")
                    #     if logger_data:
                    #         for key, value in logger_data.items():
                    #             print(f"  {key}: {value}")
                    #     else:
                    #         print("  Logger数据为空!")
                    #     
                    #     print(f"🔍 Training Stats调试:")
                    #     for key, value in self.training_stats.items():
                    #         if value != 0:  # 只显示非零值
                    #             print(f"  {key}: {value}")
                    #     print()
                    
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
                    
                    # 更新进度条
                    if hasattr(self, 'progress') and self.progress_task is not None:
                        self.progress.update(self.progress_task, completed=self.num_timesteps)
                    
                    # 创建布局组件
                    reward_panel, tensorboard_panel, status_panel = self._create_training_layout()
                    
                    # 清屏并显示
                    self.console.clear()
                    
                    # 显示上排面板
                    top_row = Columns([reward_panel, tensorboard_panel], equal=True)
                    self.console.print(top_row)
                    self.console.print("")
                    
                    # 显示训练统计表格
                    stats_panel = Panel(
                        self.training_table,
                        title="训练统计",
                        border_style="blue"
                    )
                    self.console.print(stats_panel)
                    self.console.print("")
                    
                    # 显示状态信息
                    self.console.print(status_panel)
                
                return super()._on_step()
            
            def _create_training_layout(self):
                """创建训练界面布局"""
                # 奖励系统信息面板
                reward_type = "增强奖励" if self.use_enhanced_reward else "标准奖励"
                reward_icon = "🎯" if self.use_enhanced_reward else "📊"
                reward_desc = "密集奖励信号，多维度反馈" if self.use_enhanced_reward else "经典强化学习奖励"
                
                reward_panel = Panel(
                    f"[bold green]{reward_icon} {reward_type}系统[/bold green]\n[dim]{reward_desc}[/dim]",
                    title="奖励系统",
                    border_style="green"
                )
                
                # TensorBoard提示
                tensorboard_panel = Panel(
                    "[bold cyan]📊 TensorBoard监控[/bold cyan]\n" +
                    "[dim]在新终端中运行以下命令查看详细训练曲线:[/dim]\n" +
                    "[yellow]tensorboard --logdir ./tensorboard_logs[/yellow]\n" +
                    "[dim]然后在浏览器中访问: http://localhost:6006[/dim]",
                    title="监控工具",
                    border_style="cyan"
                )
                
                # 状态信息
                progress_percent = (self.num_timesteps / self.total_timesteps) * 100
                status_lines = []
                status_lines.append(f"[bold blue]训练进度[/bold blue]: {progress_percent:.1f}% ({self.num_timesteps:,}/{self.total_timesteps:,} steps)")
                
                if len(self.episode_scores) > 0:
                    latest_score = self.episode_scores[-1]
                    avg_score = np.mean(self.episode_scores)
                    status_lines.append(f"[bold yellow]Episode信息[/bold yellow]: 最新分数={latest_score:.0f}, 平均分数={avg_score:.0f}")
                
                if self.use_enhanced_reward:
                    status_lines.append("[dim]💡 增强奖励提供更密集的学习信号[/dim]")
                
                status_panel = Panel(
                    "\n".join(status_lines),
                    title="状态信息",
                    border_style="magenta"
                )
                
                # 创建完整布局并返回所有组件
                return reward_panel, tensorboard_panel, status_panel
        
        callback = RichTrainingCallback(
            eval_freq=eval_freq, 
            save_freq=save_freq, 
            total_timesteps=total_timesteps,
            auto_save_freq=auto_save_freq,
            checkpoint_freq=checkpoint_freq,
            enable_lr_decay=enable_lr_decay
        )
        
        # 开始训练
        print(f"📈 开始使用 {algorithm} 训练，目标步数: {total_timesteps}")
        print(f"🎯 奖励系统: {'增强奖励' if use_enhanced_reward else '标准奖励'}")
        print("� TensorBoard日志: ./tensorboard_logs/")
        print("�💡 训练界面将每5秒更新一次...")
        print("\n🚀 启动 TensorBoard 监控:")
        print("   在新终端中运行: tensorboard --logdir ./tensorboard_logs")
        print("   然后访问: http://localhost:6006")
        print("\n" + "="*60)
        start_time = time.time()
        
        # 启动进度条
        with callback.progress:
            model.learn(
                total_timesteps=total_timesteps,
                callback=callback,
                tb_log_name=f"{algorithm}_gobigger_{'enhanced' if use_enhanced_reward else 'standard'}"
            )
        
        train_time = time.time() - start_time
        
        # 训练完成信息
        callback.console.clear()
        completion_panel = Panel(
            f"[bold green]✅ 训练成功完成！[/bold green]\n\n" +
            f"[yellow]训练时间[/yellow]: {train_time:.2f}秒 ({train_time/60:.1f}分钟)\n" +
            f"[yellow]总步数[/yellow]: {total_timesteps:,}\n" +
            f"[yellow]奖励系统[/yellow]: {'增强奖励' if use_enhanced_reward else '标准奖励'}\n" +
            f"[yellow]算法[/yellow]: {algorithm}\n\n" +
            f"[cyan]📊 查看详细训练曲线:[/cyan]\n" +
            f"[dim]tensorboard --logdir ./tensorboard_logs[/dim]\n\n" +
            f"[cyan]📁 模型文件:[/cyan]\n" +
            f"[dim]./models/ 和 ./checkpoints/[/dim]",
            title="🎉 训练完成",
            border_style="green"
        )
        callback.console.print(completion_panel)
        
    else:
        # 传统文本界面训练
        callback = TrainingCallback(
            eval_freq=eval_freq, 
            save_freq=save_freq, 
            total_timesteps=total_timesteps,
            auto_save_freq=auto_save_freq,
            checkpoint_freq=checkpoint_freq,
            enable_lr_decay=enable_lr_decay
        )
        callback.use_enhanced_reward = use_enhanced_reward
        
        # 设置初始学习率（用于学习率衰减）
        if hasattr(model, 'learning_rate'):
            if callable(model.learning_rate):
                callback.initial_lr = model.learning_rate(1.0)
            else:
                callback.initial_lr = model.learning_rate
        
        reward_info = "增强奖励系统" if use_enhanced_reward else "标准奖励系统"
        if total_timesteps >= 3000000:
            training_mode = "超长时间"
        elif total_timesteps >= 1000000:
            training_mode = "长时间"
        elif total_timesteps >= 500000:
            training_mode = "中长时间"
        else:
            training_mode = "标准"
        print(f"📈 开始{training_mode}训练，目标步数: {total_timesteps:,} ({reward_info})")
        if enable_lr_decay:
            print(f"📊 学习率衰减: 启用 (初始LR: {callback.initial_lr})")
        print(f"💾 保存设置: 检查点每{checkpoint_freq:,}步, 自动保存每{auto_save_freq:,}步")
        if total_timesteps >= 3000000:
            print(f"⏰ 预计训练时间: 20-30小时 (建议周末进行)")
        print("📊 TensorBoard监控:")
        print("   在新终端运行: tensorboard --logdir ./tensorboard_logs")
        print("   访问: http://localhost:6006")
        print("="*50)
        start_time = time.time()
        
        model.learn(
            total_timesteps=total_timesteps,
            callback=callback,
            tb_log_name=f"{algorithm}_gobigger_{'enhanced' if use_enhanced_reward else 'standard'}"
        )
        
        train_time = time.time() - start_time
        print(f"\n✅ 训练完成！用时: {train_time:.2f}秒 ({train_time/60:.1f}分钟)")
        print("📊 查看训练曲线: tensorboard --logdir ./tensorboard_logs")
    
    # 保存最终模型
    reward_suffix = "_enhanced" if use_enhanced_reward else "_standard"
    final_model_path = f"models/{algorithm}_gobigger{reward_suffix}_final.zip"
    os.makedirs("models", exist_ok=True)
    model.save(final_model_path)
    print(f"💾 最终模型已保存: {final_model_path}")
    
    return model

def simple_random_training(episodes=100, use_enhanced_reward=False):
    """简单的随机策略演示（支持增强奖励系统）"""
    reward_type = "增强奖励" if use_enhanced_reward else "标准奖励"
    print(f"🎮 运行随机策略演示训练 ({reward_type})...")
    
    if use_enhanced_reward:
        env = create_enhanced_env({'max_episode_steps': 500})
        print("✨ 使用增强奖励系统进行演示")
    else:
        env = create_env({'max_episode_steps': 500})
        print("📊 使用标准奖励系统进行演示")
    
    episode_rewards = []
    episode_lengths = []
    
    for episode in range(episodes):
        obs, info = env.reset()
        total_reward = 0
        steps = 0
        
        while True:
            # 随机动作
            action = env.action_space.sample()
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
                        
                        # 如果使用增强奖励，显示奖励组件信息
                        if use_enhanced_reward and hasattr(env, 'reward_components_history') and env.reward_components_history:
                            latest_components = env.reward_components_history[-1]['components']
                            if latest_components:
                                print(f"    奖励组件: ", end="")
                                for comp, val in latest_components.items():
                                    if abs(val) > 0.001:
                                        print(f"{comp}={val:.3f} ", end="")
                                print()
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
    title = f'Episode Rewards ({reward_type})'
    plt.title(title)
    plt.xlabel('Episode')
    plt.ylabel('Total Reward')
    
    plt.subplot(1, 2, 2)
    plt.plot(episode_lengths)
    plt.title('Episode Lengths')
    plt.xlabel('Episode')
    plt.ylabel('Steps')
    
    plt.tight_layout()
    filename = f'random_training_results_{reward_type.lower().replace(" ", "_")}.png'
    plt.savefig(filename)
    plt.show()
    
    print(f"📊 训练结果已保存到 {filename}")
    print(f"📈 最终平均奖励: {np.mean(episode_rewards[-20:]):.3f}")
    
    if use_enhanced_reward:
        print("💡 增强奖励系统提供了更密集的奖励信号")

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
    print("🤖 GoBigger 强化学习训练器 (支持增强奖励)")
    print("🔥 Gemini优化版本 - 解决策略崩塌问题")
    print("=" * 60)
    
    # 显示Gemini优化信息
    if not RICH_AVAILABLE:
        print("🎯 Gemini分析优化亮点:")
        print("  ✅ 降低n_epochs (20→6-8) - 防止过拟合导致的策略崩塌")
        print("  ✅ 提高ent_coef (0.005→0.01+) - 增加策略多样性和探索")
        print("  ✅ 事件驱动奖励 - 大幅奖励Split/Eject高级动作")
        print("  ✅ 平衡clip_range - 提高学习灵活性")
        print("  🔥 目标：智能体学会多样化策略，使用所有动作类型")
        print()
    else:
        console = Console()
        optimization_panel = Panel(
            "[bold green]🔥 Gemini AI 深度分析优化[/bold green]\n\n" +
            "[yellow]🎯 策略崩塌问题诊断[/yellow]:\n" +
            "  • 超高n_epochs(20)导致严重过拟合\n" +
            "  • 低ent_coef(0.005)抑制探索多样性\n" +
            "  • 智能体只会单一方向移动+从不Split/Eject\n\n" +
            "[yellow]✨ 优化措施[/yellow]:\n" +
            "  • [green]降低n_epochs(20→6-8)[/green] - 防止过拟合\n" +
            "  • [green]提高ent_coef(0.005→0.01+)[/green] - 增加策略多样性\n" +
            "  • [green]事件驱动奖励[/green] - 重奖Split(+2.0)/Eject(+1.5)动作\n" +
            "  • [green]平衡clip_range[/green] - 提高学习灵活性\n\n" +
            "[bold cyan]🎯 预期效果[/bold cyan]: 智能体学会多样化策略，主动使用所有动作类型",
            title="Gemini AI 优化版本",
            border_style="red"
        )
        console.print(optimization_panel)
        print()
    
    if not RICH_AVAILABLE:
        print("💡 建议安装 rich 库获得更好的训练界面: pip install rich")
        print("   当前使用传统文本界面")
        print()
    else:
        console = Console()
        welcome_panel = Panel(
            "[bold green]🎉 欢迎使用 GoBigger RL 训练器[/bold green]\n\n" +
            "[yellow]✨ Rich界面支持[/yellow]: 美化界面、进度条、实时统计\n" +
            "[yellow]📊 TensorBoard集成[/yellow]: 详细训练曲线监控\n" +
            "[yellow]🎯 增强奖励系统[/yellow]: 多维度密集奖励信号\n" +
            "[yellow]🚀 多算法支持[/yellow]: PPO、DQN、A2C\n" +
            "[yellow]⏰ 长时间训练优化[/yellow]: 自动保存、学习率衰减、网络优化",
            title="功能特色",
            border_style="green"
        )
        console.print(welcome_panel)
        print()
    
    # 训练配置
    config = {
        'max_episode_steps': 1500,  # 每局最大步数
    }
    
    if STABLE_BASELINES_AVAILABLE:
        print("🎯 选择训练模式 (Gemini优化版本):")
        print("1. PPO + 标准奖励 - 经典强化学习 + 事件驱动奖励")
        print("2. PPO + 增强奖励 - 密集奖励信号 + Split/Eject激励 (推荐)")
        print("3. 🌙 PPO + 增强奖励 - 长时间训练 (2M步, 一整晚)")
        print("4. 🌟 PPO + 增强奖励 - 超长时间训练 (4M步, 周末训练)")
        print("5. DQN + 标准奖励")
        print("6. DQN + 增强奖励")
        print("7. A2C + 标准奖励")
        print("8. A2C + 增强奖励") 
        print("9. 评估现有模型")
        print("10. 随机策略演示 (标准奖励)")
        print("11. 随机策略演示 (增强奖励)")
        print("")
        print("💡 Gemini优化说明:")
        print("   - 所有PPO模式均已优化超参数，防止策略崩塌")
        print("   - 增加了Split(+2.0)和Eject(+1.5)动作的事件奖励")
        print("   - 提高探索多样性，避免单一方向移动")
        
        choice = input("\n请选择 (1-11): ").strip()
        
        if choice == '1':
            model = train_with_stable_baselines3('PPO', total_timesteps=1000000, config=config, use_enhanced_reward=False)
        elif choice == '2':
            model = train_with_stable_baselines3('PPO', total_timesteps=1000000, config=config, use_enhanced_reward=True)
        elif choice == '3':
            print("🌙 长时间训练模式 - 适合一整晚训练")
            print("   - 训练步数: 2,000,000 (约8-12小时)")
            print("   - 网络结构: 深度优化 [256, 256, 128]")
            print("   - 学习率衰减: 启用")
            print("   - 自动保存: 每10K步") 
            print("   - 检查点: 每50K步")
            confirm = input("确认开始长时间训练? (y/N): ").strip().lower()
            if confirm == 'y':
                model = train_with_stable_baselines3('PPO', total_timesteps=2000000, config=config, use_enhanced_reward=True)
            else:
                print("❌ 取消训练")
                return
        elif choice == '4':
            print("🌟 超长时间训练模式 - 适合周末训练")
            print("   - 训练步数: 4,000,000 (约20-30小时)")
            print("   - 网络结构: 深度优化 [512, 512, 256, 128]")
            print("   - 学习率衰减: 启用 (更保守的衰减)")
            print("   - 自动保存: 每25K步")
            print("   - 检查点: 每100K步")
            print("   - 超参数: 专为长期训练优化")
            print("   ⚠️  请确保有足够的时间和计算资源")
            confirm = input("确认开始超长时间训练? (y/N): ").strip().lower()
            if confirm == 'y':
                model = train_with_stable_baselines3('PPO', total_timesteps=4000000, config=config, use_enhanced_reward=True)
            else:
                print("❌ 取消训练")
                return
        elif choice == '5':
            model = train_with_stable_baselines3('DQN', total_timesteps=50000, config=config, use_enhanced_reward=False)
        elif choice == '6':
            model = train_with_stable_baselines3('DQN', total_timesteps=50000, config=config, use_enhanced_reward=True)
        elif choice == '7':
            model = train_with_stable_baselines3('A2C', total_timesteps=50000, config=config, use_enhanced_reward=False)
        elif choice == '8':
            model = train_with_stable_baselines3('A2C', total_timesteps=50000, config=config, use_enhanced_reward=True)
        elif choice == '9':
            model_path = input("请输入模型路径: ").strip()
            if os.path.exists(model_path):
                evaluate_model(model_path)
            else:
                print("❌ 模型文件不存在")
        elif choice == '10':
            simple_random_training(episodes=50, use_enhanced_reward=False)
        elif choice == '11':
            simple_random_training(episodes=50, use_enhanced_reward=True)
        else:
            print("❌ 无效选择")
            
    else:
        print("🎯 选择演示模式:")
        print("1. 随机策略演示 (标准奖励)")
        print("2. 随机策略演示 (增强奖励)")
        
        choice = input("\n请选择 (1-2): ").strip()
        
        if choice == '1':
            simple_random_training(episodes=50, use_enhanced_reward=False)
        elif choice == '2':
            simple_random_training(episodes=50, use_enhanced_reward=True)
        else:
            print("❌ 无效选择")
    
    print("\n🎉 训练完成！")
    print("💡 长时间训练提示：")
    print("  - 📊 查看训练曲线: tensorboard --logdir ./tensorboard_logs")
    print("  - 🌐 TensorBoard访问: http://localhost:6006")
    print("  - 📁 模型保存位置: ./models/")
    print("  - 💾 检查点位置: ./checkpoints/")
    print("  - 🔄 自动保存: 每10K步自动保存，防止意外丢失")
    print("  - 📈 学习率衰减: 长时间训练会自动优化学习率")
    print("  - 🎯 增强奖励系统提供更密集的学习信号，推荐用于新训练")
    print("  - 🌙 长时间训练模式适合一整晚训练，获得更好的性能")
    if not RICH_AVAILABLE:
        print("  - ✨ 建议安装 rich 库享受更好的训练界面体验: pip install rich")

if __name__ == "__main__":
    main()
