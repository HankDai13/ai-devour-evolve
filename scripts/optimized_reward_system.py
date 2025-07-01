#!/usr/bin/env python3
"""
优化奖励函数设计 - 解决"一条路走到黑"问题
===============================================

实现三大核心优化：
1. 新区域探索奖励（Novelty Bonus）- 鼓励多方向探索
2. 智能分裂策略奖励 - 减少盲目分裂，鼓励通过分裂获得更多食物
3. 优化训练超参数 - 提高熵系数，增强探索性

作者: AI Assistant
日期: 2025年7月1日
"""

import numpy as np
import math
from typing import Dict, Tuple, Set, Optional, List, Any
from dataclasses import dataclass
from collections import deque, defaultdict

@dataclass
class ExplorationState:
    """探索状态追踪"""
    visited_cells: Set[Tuple[int, int]]
    cell_visit_count: Dict[Tuple[int, int], int]
    cell_size: int = 100  # 每个格子的像素大小
    max_visits_bonus: int = 3  # 最大访问次数奖励
    
class OptimizedRewardCalculator:
    """优化的奖励计算器 - 专注解决探索和策略问题"""
    
    def __init__(self, config: Optional[Dict] = None):
        self.config = config or {}
        
        # === 核心奖励权重配置 ===
        self.weights = {
            # 基础奖励
            'score_growth': 1.0,           # 分数增长基础奖励
            'survival': 0.01,              # 生存奖励
            'death_penalty': -50.0,        # 死亡重惩罚
            
            # === 探索奖励系统 ===
            'exploration_bonus': 2.0,      # 新区域探索奖励（关键）
            'diversity_bonus': 1.0,        # 移动方向多样性奖励
            'backtrack_penalty': -0.5,     # 反复回到同一区域惩罚
            
            # === 智能分裂奖励系统 ===
            'smart_split_bonus': 3.0,      # 智能分裂奖励
            'waste_split_penalty': -2.0,   # 浪费分裂惩罚
            'split_efficiency': 2.0,       # 分裂后食物获取效率奖励
            
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
        self.direction_diversity_window = 10      # 方向多样性计算窗口
        
        # === 分裂策略追踪 ===
        self.split_history = []
        self.last_split_step = -1
        self.split_efficiency_window = 50  # 分裂效率评估窗口
        
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
    
    def calculate_reward(self, current_state, previous_state, action, step_info: Dict = None) -> Tuple[float, Dict[str, float]]:
        """
        计算优化的奖励
        
        Args:
            current_state: 当前游戏状态
            previous_state: 前一游戏状态  
            action: 执行的动作
            step_info: 额外的步骤信息
            
        Returns:
            (总奖励, 奖励分解字典)
        """
        self.step_count += 1
        reward_components = {}
        
        # 1. 基础奖励（分数增长、生存、死亡）
        reward_components.update(self._calculate_basic_rewards(current_state, previous_state))
        
        # 2. 🔥 新区域探索奖励（解决"一条路走到黑"的核心）
        reward_components.update(self._calculate_exploration_rewards(current_state))
        
        # 3. 🔥 移动方向多样性奖励
        reward_components.update(self._calculate_movement_diversity_rewards(action))
        
        # 4. 🔥 智能分裂策略奖励
        reward_components.update(self._calculate_smart_split_rewards(current_state, previous_state, action))
        
        # 5. 食物获取效率奖励
        reward_components.update(self._calculate_food_efficiency_rewards(current_state, previous_state))
        
        # 计算总奖励
        total_reward = sum(
            self.weights.get(component, 1.0) * value 
            for component, value in reward_components.items()
        )
        
        # 更新历史和统计
        self._update_history(current_state, reward_components)
        self._update_stats(reward_components)
        
        return total_reward, reward_components
    
    def _calculate_basic_rewards(self, current_state, previous_state) -> Dict[str, float]:
        """计算基础奖励：分数增长、生存、死亡"""
        rewards = {}
        
        # 分数增长奖励
        if previous_state is not None:
            score_delta = getattr(current_state, 'score', 0) - getattr(previous_state, 'score', 0)
            if score_delta > 0:
                # 非线性分数奖励，鼓励更大的增长
                rewards['score_growth'] = math.sqrt(score_delta) / 20.0
            else:
                rewards['score_growth'] = score_delta / 100.0
        else:
            rewards['score_growth'] = 0.0
        
        # 生存奖励
        rewards['survival'] = 1.0
        
        # 死亡检测（假设状态包含死亡信息）
        if hasattr(current_state, 'is_dead') and current_state.is_dead:
            rewards['death_penalty'] = 1.0  # 权重中已设为负值
        else:
            rewards['death_penalty'] = 0.0
            
        return rewards
    
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
        
        # 检测是否进行了分裂
        split_action = False
        if len(action) >= 3:
            split_action = action[2] > 1.5  # 假设第三个动作是分裂信号
        
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
            
            # 智能分裂条件：
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
                # 智能分裂奖励
                food_density_bonus = min(food_nearby / 10.0, 1.0)  # 食物密度奖励
                timing_bonus = min(steps_since_last_split / 100.0, 0.5)  # 时机奖励
                rewards['smart_split_bonus'] = 1.0 + food_density_bonus + timing_bonus
                
                self.stats['efficient_splits'] += 1
            else:
                # 浪费分裂惩罚
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
    
    def _update_history(self, current_state, reward_components):
        """更新历史记录"""
        self.history.append({
            'step': self.step_count,
            'state': current_state,
            'rewards': reward_components.copy(),
            'total_reward': sum(
                self.weights.get(k, 1.0) * v for k, v in reward_components.items()
            )
        })
    
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
            'exploration_coverage': len(self.exploration.visited_cells),  # 可以后续标准化
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


def create_optimized_training_config():
    """
    创建优化的训练配置
    
    重点：提高熵系数以增强探索性
    """
    return {
        # 🔥 强化学习超参数优化（重点是熵系数）
        'learning_rate': 3e-4,
        'ent_coef': 0.05,  # 🔥 提高熵系数：从默认0.01提升到0.05，增强探索
        'vf_coef': 0.5,
        'max_grad_norm': 0.5,
        'gamma': 0.99,
        'gae_lambda': 0.95,
        'n_epochs': 10,
        'batch_size': 64,
        
        # 🔥 奖励系统配置
        'use_optimized_reward': True,
        'reward_config': {
            'exploration_cell_size': 80,  # 适中的格子大小，不会太稀疏也不会太密集
            'direction_diversity_window': 15,  # 方向多样性评估窗口
            'split_efficiency_window': 60,    # 分裂效率评估窗口
        },
        
        # 环境配置
        'max_episode_steps': 3000,  # 更长的episode以便充分探索
        'frame_skip': 1,
        
        # 训练配置
        'total_timesteps': 1000000,
        'eval_freq': 10000,
        'save_freq': 50000,
    }


if __name__ == "__main__":
    # 测试奖励计算器
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
    
    reward1, components1 = calculator.calculate_reward(state1, None, action)
    print(f"第一个位置: 总奖励={reward1:.3f}, 探索奖励={components1.get('exploration_bonus', 0):.3f}")
    
    reward2, components2 = calculator.calculate_reward(state2, state1, action)
    print(f"新位置: 总奖励={reward2:.3f}, 探索奖励={components2.get('exploration_bonus', 0):.3f}")
    
    reward3, components3 = calculator.calculate_reward(state3, state2, action)
    print(f"回到老位置: 总奖励={reward3:.3f}, 探索奖励={components3.get('exploration_bonus', 0):.3f}, 回退惩罚={components3.get('backtrack_penalty', 0):.3f}")
    
    # 测试智能分裂
    print("\n🔀 测试智能分裂:")
    split_action = [0.1, 0.1, 2.0]  # 分裂动作
    state_with_food = MockState(3000, (300, 300), 5)  # 高分+多食物
    reward_split, components_split = calculator.calculate_reward(state_with_food, state3, split_action)
    print(f"智能分裂: 总奖励={reward_split:.3f}, 分裂奖励={components_split.get('smart_split_bonus', 0):.3f}")
    
    # 测试浪费分裂
    bad_split_state = MockState(1500, (400, 400), 1)  # 低分+少食物
    reward_bad, components_bad = calculator.calculate_reward(bad_split_state, state_with_food, split_action)
    print(f"浪费分裂: 总奖励={reward_bad:.3f}, 分裂惩罚={components_bad.get('waste_split_penalty', 0):.3f}")
    
    # 输出统计
    print("\n📊 探索统计:", calculator.get_exploration_stats())
    print("📊 分裂统计:", calculator.get_split_stats())
    
    print("\n✅ 测试完成！")
