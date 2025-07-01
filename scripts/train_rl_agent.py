#!/usr/bin/env python3
"""
使用GoBigger核心引擎训练强化学习智能体
🔥 优化版：解决"一条路走到黑"问题，智能分裂策略，提高探索能力

核心优化：
1. 新区域探索奖励（Novelty Bonus）- 鼓励多方向探索
2. 智能分裂策略奖励 - 减少盲目分裂，鼓励通过分裂获得更多食物  
3. 优化训练超参数 - 提高熵系数，增强探索性

支持多种RL算法：PPO、DQN、A2C等
"""
import sys
import os
from pathlib import Path
import numpy as np
import time
import json
import math
from collections import deque, defaultdict
import matplotlib.pyplot as plt
from typing import Dict, Tuple, Set, Optional, List, Any
from dataclasses import dataclass

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

@dataclass
class ExplorationState:
    """探索状态追踪"""
    visited_cells: Set[Tuple[int, int]]
    cell_visit_count: Dict[Tuple[int, int], int]
    cell_size: int = 100  # 每个格子的像素大小
    max_visits_bonus: int = 3  # 最大访问次数奖励

class OptimizedRewardCalculator:
    """
    🔥 优化的奖励计算器 - 解决"一条路走到黑"问题
    
    实现三大核心优化：
    1. 新区域探索奖励（Novelty Bonus）
    2. 智能分裂策略奖励
    3. 移动方向多样性奖励
    """
    
    def __init__(self, config: Optional[Dict] = None):
        self.config = config or {}
        
        # === 🔥 核心奖励权重配置（优化版本）===
        self.weights = {
            # 基础奖励
            'score_growth': 1.0,           # 分数增长基础奖励
            'survival': 0.01,              # 生存奖励
            'death_penalty': -50.0,        # 死亡重惩罚
            
            # === 🔥 探索奖励系统（解决"一条路走到黑"核心）===
            'exploration_bonus': 2.0,      # 🔥 新区域探索奖励（关键！）
            'diversity_bonus': 1.0,        # 🔥 移动方向多样性奖励
            'backtrack_penalty': -0.5,     # 🔥 反复回到同一区域惩罚
            
            # === 🔥 智能分裂奖励系统 ===
            'smart_split_bonus': 3.0,      # 🔥 智能分裂奖励
            'waste_split_penalty': -2.0,   # 🔥 浪费分裂惩罚
            'split_efficiency': 2.0,       # 🔥 分裂后食物获取效率奖励
            
            # === 其他策略奖励 ===
            'food_efficiency': 1.5,        # 食物获取效率
            'size_management': 1.0,         # 大小管理奖励
        }
        
        # === 探索状态追踪 ===
        self.exploration = ExplorationState(
            visited_cells=set(),
            cell_visit_count=defaultdict(int),
            cell_size=self.config.get('exploration_cell_size', 100)
        )
        
        # === 移动方向追踪（解决"一条路走到黑"） ===
        self.movement_history = deque(maxlen=20)  # 记录最近20步的移动方向
        self.direction_diversity_window = self.config.get('direction_diversity_window', 15)  # 方向多样性计算窗口
        
        # === 分裂策略追踪 ===
        self.split_history = []
        self.last_split_step = -1
        self.split_efficiency_window = self.config.get('split_efficiency_window', 60)  # 分裂效率评估窗口
        
        # === 历史状态缓存 ===
        self.history = deque(maxlen=100)
        self.step_count = 0
        
        # === 统计信息 ===
        self.stats = {
            'total_exploration_bonus': 0.0,
            'total_smart_split_bonus': 0.0,
            'unique_cells_visited': 0,
            'direction_changes': 0,
            'efficient_splits': 0,
            'wasted_splits': 0,
        }
    
    def calculate_optimized_reward(self, current_state, previous_state, action, base_reward: float) -> Tuple[float, Dict[str, float]]:
        """
        🔥 计算优化的奖励 - 在基础奖励基础上增加探索和策略奖励
        
        Args:
            current_state: 当前游戏状态
            previous_state: 前一游戏状态  
            action: 执行的动作
            base_reward: 基础奖励
            
        Returns:
            (优化后总奖励, 奖励分解字典)
        """
        self.step_count += 1
        reward_components = {'base_reward': base_reward}
        
        # 1. 🔥 新区域探索奖励（解决"一条路走到黑"的核心）
        reward_components.update(self._calculate_exploration_rewards(current_state))
        
        # 2. 🔥 移动方向多样性奖励
        reward_components.update(self._calculate_movement_diversity_rewards(action))
        
        # 3. 🔥 智能分裂策略奖励
        reward_components.update(self._calculate_smart_split_rewards(current_state, previous_state, action))
        
        # 4. 食物获取效率奖励
        reward_components.update(self._calculate_food_efficiency_rewards(current_state, previous_state))
        
        # 计算总奖励
        total_reward = base_reward + sum(
            self.weights.get(component, 1.0) * value 
            for component, value in reward_components.items()
            if component != 'base_reward'
        )
        
        # 更新统计
        self._update_stats(reward_components)
        
        return total_reward, reward_components
    
    def _calculate_exploration_rewards(self, current_state) -> Dict[str, float]:
        """
        🔥 核心功能：计算探索奖励，解决"一条路走到黑"问题
        
        实现思路：
        1. 将地图划分为网格
        2. 追踪访问过的格子
        3. 给访问新格子大额奖励
        4. 对重复访问给予递减奖励或惩罚
        """
        rewards = {}
        
        # 获取当前位置
        if hasattr(current_state, 'rectangle') and len(current_state.rectangle) >= 4:
            center_x = (current_state.rectangle[0] + current_state.rectangle[2]) / 2
            center_y = (current_state.rectangle[1] + current_state.rectangle[3]) / 2
        elif hasattr(current_state, 'position'):
            center_x, center_y = current_state.position[0], current_state.position[1]
        else:
            # 无法获取位置，跳过探索奖励
            rewards['exploration_bonus'] = 0.0
            rewards['backtrack_penalty'] = 0.0
            return rewards
        
        # 计算当前格子坐标
        cell_x = int(center_x // self.exploration.cell_size)
        cell_y = int(center_y // self.exploration.cell_size)
        cell_coord = (cell_x, cell_y)
        
        # 更新访问计数
        self.exploration.cell_visit_count[cell_coord] += 1
        visit_count = self.exploration.cell_visit_count[cell_coord]
        
        # 新区域探索奖励
        if cell_coord not in self.exploration.visited_cells:
            # 🔥 首次访问新区域：大额奖励
            rewards['exploration_bonus'] = 1.0  # 基础探索奖励
            self.exploration.visited_cells.add(cell_coord)
            self.stats['unique_cells_visited'] += 1
            
            # 额外奖励：如果这个新区域距离之前访问的区域较远
            if len(self.exploration.visited_cells) > 1:
                min_distance = min(
                    abs(cell_x - other_x) + abs(cell_y - other_y)
                    for other_x, other_y in self.exploration.visited_cells
                    if (other_x, other_y) != cell_coord
                )
                if min_distance >= 3:  # 距离足够远
                    rewards['exploration_bonus'] += 0.5  # 远距离探索额外奖励
        else:
            # 已访问区域：根据访问次数给予递减奖励或惩罚
            if visit_count <= self.exploration.max_visits_bonus:
                # 少量访问仍有小奖励
                rewards['exploration_bonus'] = 0.1 / visit_count
            else:
                # 过度访问给予惩罚
                rewards['backtrack_penalty'] = min(visit_count - self.exploration.max_visits_bonus, 5) * 0.1
                rewards['exploration_bonus'] = 0.0
        
        if 'backtrack_penalty' not in rewards:
            rewards['backtrack_penalty'] = 0.0
        
        return rewards
    
    def _calculate_movement_diversity_rewards(self, action) -> Dict[str, float]:
        """
        🔥 计算移动方向多样性奖励，进一步解决"一条路走到黑"问题
        """
        rewards = {'diversity_bonus': 0.0}
        
        # 记录当前移动方向
        if len(action) >= 2:
            current_direction = np.array([action[0], action[1]])
            direction_magnitude = np.linalg.norm(current_direction)
            
            if direction_magnitude > 0.1:  # 有明显移动
                normalized_direction = current_direction / direction_magnitude
                self.movement_history.append(normalized_direction)
                
                # 计算方向多样性（如果有足够的历史记录）
                if len(self.movement_history) >= self.direction_diversity_window:
                    recent_directions = list(self.movement_history)[-self.direction_diversity_window:]
                    
                    # 计算方向变化的方差作为多样性指标
                    direction_changes = []
                    for i in range(1, len(recent_directions)):
                        # 计算连续方向间的角度变化
                        dot_product = np.clip(np.dot(recent_directions[i-1], recent_directions[i]), -1, 1)
                        angle_change = np.arccos(dot_product)
                        direction_changes.append(angle_change)
                    
                    if direction_changes:
                        # 方向变化越大，多样性奖励越高
                        avg_angle_change = np.mean(direction_changes)
                        diversity_score = avg_angle_change / np.pi  # 标准化到[0,1]
                        
                        if diversity_score > 0.3:  # 足够的方向变化
                            rewards['diversity_bonus'] = diversity_score * 0.5
                            self.stats['direction_changes'] += 1
        
        return rewards
    
    def _calculate_smart_split_rewards(self, current_state, previous_state, action) -> Dict[str, float]:
        """
        🔥 计算智能分裂策略奖励
        
        减少盲目分裂，鼓励在有足够食物时分裂
        """
        rewards = {
            'smart_split_bonus': 0.0,
            'waste_split_penalty': 0.0,
            'split_efficiency': 0.0
        }
        
        # 检测是否进行了分裂（假设第三个动作是分裂信号）
        split_action = False
        if len(action) >= 3:
            split_action = action[2] > 1.5
        
        if split_action:
            current_cells = 1  # 默认值
            food_nearby = 0
            current_score = 0
            
            # 获取当前状态信息
            if hasattr(current_state, 'clone') and isinstance(current_state.clone, list):
                current_cells = len(current_state.clone)
            if hasattr(current_state, 'food') and isinstance(current_state.food, list):
                food_nearby = len(current_state.food)
            if hasattr(current_state, 'score'):
                current_score = current_state.score
            
            # 评估分裂智能程度
            steps_since_last_split = self.step_count - self.last_split_step
            
            # 🔥 智能分裂条件：
            # 1. 附近有足够食物（至少3个）
            # 2. 距离上次分裂足够久（至少30步）
            # 3. 当前分数足够高（能够承受分裂）
            # 4. 当前细胞数不过多（避免过度分裂）
            
            is_smart_split = (
                food_nearby >= 3 and  # 条件1：附近有足够食物
                steps_since_last_split >= 30 and  # 条件2：分裂间隔足够
                current_score >= 2000 and  # 条件3：分数足够
                current_cells <= 8  # 条件4：避免过度分裂
            )
            
            if is_smart_split:
                # 🔥 智能分裂奖励
                food_density_bonus = min(food_nearby / 10.0, 1.0)  # 食物密度奖励
                timing_bonus = min(steps_since_last_split / 100.0, 0.5)  # 时机奖励
                rewards['smart_split_bonus'] = 1.0 + food_density_bonus + timing_bonus
                
                self.stats['efficient_splits'] += 1
            else:
                # 🔥 浪费分裂惩罚
                penalty_factors = []
                if food_nearby < 3:
                    penalty_factors.append(0.5)  # 食物不足惩罚
                if steps_since_last_split < 30:
                    penalty_factors.append(0.3)  # 分裂过频惩罚
                if current_cells > 8:
                    penalty_factors.append(0.4)  # 过度分裂惩罚
                
                rewards['waste_split_penalty'] = sum(penalty_factors)
                self.stats['wasted_splits'] += 1
            
            # 记录分裂事件
            self.last_split_step = self.step_count
            self.split_history.append({
                'step': self.step_count,
                'food_nearby': food_nearby,
                'cells_before': current_cells,
                'smart_split': is_smart_split
            })
        
        # 分裂后效率奖励：评估分裂后是否获得了更多食物
        if previous_state and hasattr(current_state, 'score') and hasattr(previous_state, 'score'):
            score_growth = current_state.score - previous_state.score
            
            # 如果最近进行了分裂且分数有显著增长
            recent_splits = [s for s in self.split_history if self.step_count - s['step'] <= self.split_efficiency_window]
            if recent_splits and score_growth > 0:
                # 根据分裂后的分数增长给予效率奖励
                efficiency_ratio = score_growth / len(recent_splits)
                rewards['split_efficiency'] = min(efficiency_ratio / 500.0, 1.0)
        
        return rewards
    
    def _calculate_food_efficiency_rewards(self, current_state, previous_state) -> Dict[str, float]:
        """计算食物获取效率奖励"""
        rewards = {'food_efficiency': 0.0}
        
        if previous_state is None:
            return rewards
        
        # 计算分数增长（主要来自食物）
        score_growth = getattr(current_state, 'score', 0) - getattr(previous_state, 'score', 0)
        
        if score_growth > 0:
            # 基于分数增长的效率奖励
            efficiency = min(score_growth / 100.0, 2.0)  # 限制最大奖励
            rewards['food_efficiency'] = efficiency
        
        return rewards
    
    def _update_stats(self, reward_components):
        """更新统计信息"""
        if 'exploration_bonus' in reward_components and reward_components['exploration_bonus'] > 0:
            self.stats['total_exploration_bonus'] += reward_components['exploration_bonus']
        
        if 'smart_split_bonus' in reward_components and reward_components['smart_split_bonus'] > 0:
            self.stats['total_smart_split_bonus'] += reward_components['smart_split_bonus']
    
    def get_exploration_stats(self) -> Dict[str, Any]:
        """获取探索统计信息"""
        return {
            'unique_cells_visited': len(self.exploration.visited_cells),
            'total_cell_visits': sum(self.exploration.cell_visit_count.values()),
            'exploration_coverage': len(self.exploration.visited_cells),
            'avg_visits_per_cell': (
                sum(self.exploration.cell_visit_count.values()) / len(self.exploration.visited_cells)
                if self.exploration.visited_cells else 0
            ),
            'direction_diversity_score': self.stats['direction_changes'] / max(self.step_count, 1),
        }
    
    def get_split_stats(self) -> Dict[str, Any]:
        """获取分裂策略统计信息"""
        total_splits = len(self.split_history)
        return {
            'total_splits': total_splits,
            'efficient_splits': self.stats['efficient_splits'],
            'wasted_splits': self.stats['wasted_splits'],
            'split_efficiency_ratio': (
                self.stats['efficient_splits'] / total_splits
                if total_splits > 0 else 0
            ),
            'avg_split_interval': (
                self.step_count / total_splits if total_splits > 0 else float('inf')
            ),
        }
    
    def reset(self):
        """重置奖励计算器（新episode时调用）"""
        self.exploration.visited_cells.clear()
        self.exploration.cell_visit_count.clear()
        self.movement_history.clear()
        self.split_history.clear()
        self.history.clear()
        self.step_count = 0
        self.last_split_step = -1
        
        # 重置统计
        for key in self.stats:
            self.stats[key] = 0
        self.stats['unique_cells_visited'] = 0
        self.stats['direction_changes'] = 0
        self.stats['efficient_splits'] = 0
        self.stats['wasted_splits'] = 0

# 全局奖励计算器实例
_global_reward_calculator = OptimizedRewardCalculator()

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
    """创建训练环境（带Monitor和优化奖励系统）"""
    default_config = {
        'max_episode_steps': 3000,      # 🔥 更长的episode以便充分探索
        'use_enhanced_reward': False,   # 默认使用简单奖励
        'debug_rewards': False,         # 奖励调试选项
        'use_optimized_reward': True,   # 🔥 启用优化奖励系统（探索+智能分裂）
        'exploration_cell_size': 80,    # 🔥 探索网格大小（适中的格子大小）
        'direction_diversity_window': 15,  # 🔥 方向多样性评估窗口
        'split_efficiency_window': 60,     # 🔥 分裂效率评估窗口
        'reward_debug': False,          # 奖励组件调试
    }
    if config:
        default_config.update(config)
    
    # 创建环境并用Monitor包装（重要：这是episode统计的关键）
    env = GoBiggerEnv(default_config)
    
    # 🔥 如果启用优化奖励，包装环境
    if default_config.get('use_optimized_reward', True):
        env = OptimizedRewardWrapper(env, default_config)
        print("🎯 启用优化奖励系统 - 探索激励+智能分裂策略")
    
    # 🔥 启用奖励调试（如果请求）
    if default_config.get('debug_rewards', False):
        env.debug_rewards = True
        print("🔍 启用奖励调试模式 - 将显示Split/Eject动作奖励")
    
    if STABLE_BASELINES_AVAILABLE:
        env = Monitor(env)
    return env

class OptimizedRewardWrapper:
    """
    🔥 优化奖励包装器 - 在原有奖励基础上添加探索和策略奖励
    """
    
    def __init__(self, env, config):
        self.env = env
        self.config = config
        self.reward_calculator = OptimizedRewardCalculator(config)
        self.previous_state = None
        self.episode_optimized_rewards = []
        self.episode_exploration_rewards = []
        self.episode_split_rewards = []
        
        # 将环境方法委托给原环境
        for attr in ['observation_space', 'action_space', 'spec']:
            if hasattr(env, attr):
                setattr(self, attr, getattr(env, attr))
    
    def step(self, action):
        """执行一步并计算优化奖励"""
        obs, base_reward, done, info = self.env.step(action)
        
        # 🔥 计算优化奖励
        current_state = obs  # 假设obs包含状态信息
        optimized_reward, reward_components = self.reward_calculator.calculate_optimized_reward(
            current_state, self.previous_state, action, base_reward
        )
        
        # 更新状态
        self.previous_state = current_state
        
        # 统计奖励组件
        exploration_reward = reward_components.get('exploration_bonus', 0) + reward_components.get('diversity_bonus', 0)
        split_reward = reward_components.get('smart_split_bonus', 0) + reward_components.get('split_efficiency', 0)
        
        self.episode_optimized_rewards.append(optimized_reward - base_reward)
        self.episode_exploration_rewards.append(exploration_reward)
        self.episode_split_rewards.append(split_reward)
        
        # 🔥 在info中添加奖励调试信息
        if self.config.get('reward_debug', False):
            info['reward_components'] = reward_components
            info['optimized_reward_bonus'] = optimized_reward - base_reward
        
        # episode结束时添加统计信息
        if done:
            info['exploration_stats'] = self.reward_calculator.get_exploration_stats()
            info['split_stats'] = self.reward_calculator.get_split_stats()
            info['total_exploration_reward'] = sum(self.episode_exploration_rewards)
            info['total_split_reward'] = sum(self.episode_split_rewards)
            info['total_optimized_bonus'] = sum(self.episode_optimized_rewards)
            
            # 重置episode统计
            self.episode_optimized_rewards = []
            self.episode_exploration_rewards = []
            self.episode_split_rewards = []
            self.reward_calculator.reset()
            self.previous_state = None
        
        return obs, optimized_reward, done, info
    
    def reset(self):
        """重置环境"""
        obs = self.env.reset()
        self.previous_state = None
        self.reward_calculator.reset()
        self.episode_optimized_rewards = []
        self.episode_exploration_rewards = []
        self.episode_split_rewards = []
        return obs
    
    def render(self, mode='human'):
        """渲染环境"""
        return self.env.render(mode)
    
    def close(self):
        """关闭环境"""
        return self.env.close()
    
    def __getattr__(self, name):
        """委托其他属性给原环境"""
        return getattr(self.env, name)

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
    
    # 🎯 长时间训练优化的超参数（🔥 重点提高熵系数增强探索）
    def get_optimized_hyperparams(algorithm, total_timesteps):
        """根据算法和训练规模优化超参数 - 重点提高熵系数解决探索问题"""
        if algorithm == 'PPO':
            if total_timesteps >= 3000000:  # 超超长时间训练（400万步级别）
                return {
                    "learning_rate": 2e-4,  # 更低的学习率，更稳定
                    "n_steps": 2048,  # 更大的rollout，更好的采样效率
                    "batch_size": 256,  # 更大的批量，更稳定的梯度
                    "n_epochs": 6,  # 🔥 减少epochs防止过拟合！
                    "gamma": 0.998,  # 更高的折扣因子，考虑长期回报
                    "gae_lambda": 0.99,  # 更高的GAE参数
                    "clip_range": 0.15,  # 稍大的裁剪范围，增加学习灵活性
                    "ent_coef": 0.1,   # 🔥🔥🔥 超高熵系数！最强探索设置，解决"一条路走到黑"
                    "vf_coef": 0.25,  # 价值函数系数
                    "max_grad_norm": 0.5  # 稍微放松梯度裁剪
                }
            elif total_timesteps >= 1000000:  # 超长时间训练
                return {
                    "learning_rate": 2.5e-4,  # 稍低的学习率
                    "n_steps": 1024,  # 更大的rollout
                    "batch_size": 128,  # 更大的批量
                    "n_epochs": 8,  # 🔥 减少epochs防止过拟合！
                    "gamma": 0.995,  # 更高的折扣因子
                    "gae_lambda": 0.98,  # GAE参数
                    "clip_range": 0.18,  # 稍大的裁剪范围
                    "ent_coef": 0.08,  # 🔥🔥🔥 超高熵系数！强力探索设置
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
                    "ent_coef": 0.06,  # 🔥🔥🔥 高熵系数，强化探索多样性
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
                    "ent_coef": 0.05,  # 🔥🔥🔥 显著提高熵系数，强化探索
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

def test_optimized_reward_system():
    """
    🧪 测试优化奖励系统的功能
    """
    print("🧪 测试优化奖励计算器...")
    
    calculator = OptimizedRewardCalculator()
    
    # 模拟一些状态用于测试
    class MockState:
        def __init__(self, score, position, food_count=0):
            self.score = score
            self.rectangle = [position[0]-10, position[1]-10, position[0]+10, position[1]+10]
            self.food = [f"food_{i}" for i in range(food_count)]
            self.clone = ["main_cell"]
    
    # 测试探索奖励
    print("\n📍 测试探索奖励:")
    state1 = MockState(1000, (100, 100), 2)  # 新位置
    state2 = MockState(1100, (200, 100), 3)  # 另一个新位置
    state3 = MockState(1200, (100, 100), 1)  # 回到第一个位置
    
    action = [0.5, 0.3, 0]  # 向右上移动，不分裂
    
    reward1, components1 = calculator.calculate_optimized_reward(state1, None, action, 10.0)
    print(f"第一个位置: 总奖励={reward1:.3f}, 探索奖励={components1.get('exploration_bonus', 0):.3f}")
    
    reward2, components2 = calculator.calculate_optimized_reward(state2, state1, action, 15.0)
    print(f"新位置: 总奖励={reward2:.3f}, 探索奖励={components2.get('exploration_bonus', 0):.3f}")
    
    reward3, components3 = calculator.calculate_optimized_reward(state3, state2, action, 12.0)
    print(f"回到老位置: 总奖励={reward3:.3f}, 探索奖励={components3.get('exploration_bonus', 0):.3f}, 回退惩罚={components3.get('backtrack_penalty', 0):.3f}")
    
    # 测试智能分裂
    print("\n🔀 测试智能分裂:")
    split_action = [0.1, 0.1, 2.0]  # 分裂动作
    state_with_food = MockState(3000, (300, 300), 5)  # 高分+多食物
    reward_split, components_split = calculator.calculate_optimized_reward(state_with_food, state3, split_action, 8.0)
    print(f"智能分裂: 总奖励={reward_split:.3f}, 分裂奖励={components_split.get('smart_split_bonus', 0):.3f}")
    
    # 测试浪费分裂
    bad_split_state = MockState(1500, (400, 400), 1)  # 低分+少食物
    reward_bad, components_bad = calculator.calculate_optimized_reward(bad_split_state, state_with_food, split_action, 5.0)
    print(f"浪费分裂: 总奖励={reward_bad:.3f}, 分裂惩罚={components_bad.get('waste_split_penalty', 0):.3f}")
    
    # 输出统计
    print("\n📊 探索统计:", calculator.get_exploration_stats())
    print("📊 分裂统计:", calculator.get_split_stats())
    
    print("\n✅ 优化奖励系统测试完成！")

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

def test_optimized_reward_system():
    """
    🧪 测试优化奖励系统的功能
    """
    print("🧪 测试优化奖励计算器...")
    
    calculator = OptimizedRewardCalculator()
    
    # 模拟一些状态用于测试
    class MockState:
        def __init__(self, score, position, food_count=0):
            self.score = score
            self.rectangle = [position[0]-10, position[1]-10, position[0]+10, position[1]+10]
            self.food = [f"food_{i}" for i in range(food_count)]
            self.clone = ["main_cell"]
    
    # 测试探索奖励
    print("\n📍 测试探索奖励:")
    state1 = MockState(1000, (100, 100), 2)  # 新位置
    state2 = MockState(1100, (200, 100), 3)  # 另一个新位置
    state3 = MockState(1200, (100, 100), 1)  # 回到第一个位置
    
    action = [0.5, 0.3, 0]  # 向右上移动，不分裂
    
    reward1, components1 = calculator.calculate_optimized_reward(state1, None, action, 10.0)
    print(f"第一个位置: 总奖励={reward1:.3f}, 探索奖励={components1.get('exploration_bonus', 0):.3f}")
    
    reward2, components2 = calculator.calculate_optimized_reward(state2, state1, action, 15.0)
    print(f"新位置: 总奖励={reward2:.3f}, 探索奖励={components2.get('exploration_bonus', 0):.3f}")
    
    reward3, components3 = calculator.calculate_optimized_reward(state3, state2, action, 12.0)
    print(f"回到老位置: 总奖励={reward3:.3f}, 探索奖励={components3.get('exploration_bonus', 0):.3f}, 回退惩罚={components3.get('backtrack_penalty', 0):.3f}")
    
    # 测试智能分裂
    print("\n🔀 测试智能分裂:")
    split_action = [0.1, 0.1, 2.0]  # 分裂动作
    state_with_food = MockState(3000, (300, 300), 5)  # 高分+多食物
    reward_split, components_split = calculator.calculate_optimized_reward(state_with_food, state3, split_action, 8.0)
    print(f"智能分裂: 总奖励={reward_split:.3f}, 分裂奖励={components_split.get('smart_split_bonus', 0):.3f}")
    
    # 测试浪费分裂
    bad_split_state = MockState(1500, (400, 400), 1)  # 低分+少食物
    reward_bad, components_bad = calculator.calculate_optimized_reward(bad_split_state, state_with_food, split_action, 5.0)
    print(f"浪费分裂: 总奖励={reward_bad:.3f}, 分裂惩罚={components_bad.get('waste_split_penalty', 0):.3f}")
    
    # 输出统计
    print("\n📊 探索统计:", calculator.get_exploration_stats())
    print("📊 分裂统计:", calculator.get_split_stats())
    
    print("\n✅ 优化奖励系统测试完成！")

if __name__ == "__main__":
    # 检查是否需要运行测试
    import sys
    if len(sys.argv) > 1 and sys.argv[1] == "--test-reward":
        test_optimized_reward_system()
    else:
        main()
