#!/usr/bin/env python3
"""
增强奖励系统 - 优化GoBigger强化学习的奖励机制
===================================================

此脚本提供了一个更加细致的奖励系统，专门设计来：
1. 鼓励更高效的食物消费
2. 奖励适时的分裂策略
3. 平衡探索与利用
4. 提供更密集的奖励信号

作者: AI Assistant
日期: 2024年
"""

import numpy as np
import math

class EnhancedRewardCalculator:
    """增强奖励计算器"""
    
    def __init__(self, config=None):
        self.config = config or {}
        
        # 奖励权重配置（可调参数）
        self.weights = {
            'score_growth': 2.0,        # 分数增长奖励权重
            'efficiency': 1.0,          # 效率奖励权重
            'exploration': 0.5,         # 探索奖励权重
            'strategic_split': 1.5,     # 战略分裂奖励权重
            'food_density': 0.8,        # 食物密度奖励权重
            'survival': 0.02,           # 生存奖励权重
            'time_penalty': -0.001,     # 时间惩罚权重
            'death_penalty': -15.0,     # 死亡惩罚权重
        }
        
        # 历史状态追踪
        self.history = []
        self.moving_avg_score = 0.0
        self.moving_avg_window = 10
        
        # 战略行为追踪
        self.consecutive_growth_steps = 0
        self.last_split_step = -1
        self.food_eaten_count = 0
        
        # 区域探索追踪
        self.visited_regions = set()
        self.region_size = 200  # 区域大小（用于探索奖励）
        
    def reset(self):
        """重置奖励计算器状态"""
        self.history.clear()
        self.moving_avg_score = 0.0
        self.consecutive_growth_steps = 0
        self.last_split_step = -1
        self.food_eaten_count = 0
        self.visited_regions.clear()
        
    def calculate_reward(self, current_state, previous_state, action, step_count):
        """
        计算增强奖励
        
        Args:
            current_state: 当前游戏状态
            previous_state: 上一步游戏状态  
            action: 执行的动作
            step_count: 当前步数
            
        Returns:
            float: 计算得出的奖励值
        """
        if not current_state or not current_state.player_states:
            return self.weights['death_penalty']
        
        ps_current = list(current_state.player_states.values())[0]
        
        # 获取基础信息
        if previous_state and previous_state.player_states:
            ps_previous = list(previous_state.player_states.values())[0]
        else:
            ps_previous = None
            
        reward_components = {}
        
        # 1. 分数增长奖励（非线性，鼓励持续增长）
        reward_components['score_growth'] = self._calculate_score_growth_reward(
            ps_current, ps_previous)
            
        # 2. 效率奖励（分数增长与移动距离的比率）
        reward_components['efficiency'] = self._calculate_efficiency_reward(
            ps_current, ps_previous, action)
            
        # 3. 探索奖励（鼓励访问新区域）
        reward_components['exploration'] = self._calculate_exploration_reward(
            ps_current)
            
        # 4. 战略分裂奖励（在合适时机分裂）
        reward_components['strategic_split'] = self._calculate_strategic_split_reward(
            ps_current, ps_previous, action, step_count)
            
        # 5. 食物密度奖励（在食物密集区域获得奖励）
        reward_components['food_density'] = self._calculate_food_density_reward(
            ps_current)
            
        # 6. 基础奖励（生存、时间惩罚、死亡惩罚）
        reward_components.update(self._calculate_basic_rewards(current_state))
        
        # 计算总奖励
        total_reward = 0.0
        for component, value in reward_components.items():
            total_reward += self.weights.get(component, 1.0) * value
            
        # 更新历史状态
        self._update_history(ps_current, reward_components, step_count)
        
        return total_reward, reward_components
    
    def _calculate_score_growth_reward(self, ps_current, ps_previous):
        """计算分数增长奖励（非线性）"""
        if not ps_previous:
            return 0.0
            
        score_delta = ps_current.score - ps_previous.score
        
        if score_delta > 0:
            # 非线性奖励：鼓励更大的分数增长
            reward = math.sqrt(score_delta) / 10.0
            self.consecutive_growth_steps += 1
            
            # 连续增长奖励加成
            if self.consecutive_growth_steps >= 3:
                reward *= 1.2  # 20%加成
            if self.consecutive_growth_steps >= 5:
                reward *= 1.5  # 额外50%加成
                
            self.food_eaten_count += 1
            return reward
        else:
            self.consecutive_growth_steps = 0
            return score_delta / 100.0  # 小的负奖励
    
    def _calculate_efficiency_reward(self, ps_current, ps_previous, action):
        """计算效率奖励（分数增长与行动的比率）"""
        if not ps_previous:
            return 0.0
            
        score_delta = ps_current.score - ps_previous.score
        
        # 计算移动距离（如果action包含位置信息）
        if len(action) >= 2:
            move_distance = math.sqrt(action[0]**2 + action[1]**2)
        else:
            move_distance = 1.0
        
        if score_delta > 0 and move_distance > 0:
            # 效率 = 分数增长 / 移动成本
            efficiency = score_delta / (move_distance + 0.1)  # 避免除零
            return min(efficiency / 50.0, 1.0)  # 限制最大奖励
        
        return 0.0
    
    def _calculate_exploration_reward(self, ps_current):
        """计算探索奖励（鼓励访问新区域）"""
        if not hasattr(ps_current, 'rectangle') or len(ps_current.rectangle) < 4:
            return 0.0
        
        # 获取当前位置的区域ID
        center_x = (ps_current.rectangle[0] + ps_current.rectangle[2]) / 2
        center_y = (ps_current.rectangle[1] + ps_current.rectangle[3]) / 2
        
        region_x = int(center_x // self.region_size)
        region_y = int(center_y // self.region_size)
        region_id = (region_x, region_y)
        
        # 如果是新区域，给予奖励
        if region_id not in self.visited_regions:
            self.visited_regions.add(region_id)
            return 0.5  # 探索新区域奖励
        
        return 0.0
    
    def _calculate_strategic_split_reward(self, ps_current, ps_previous, action, step_count):
        """计算战略分裂奖励"""
        if not ps_previous:
            return 0.0
        
        # 检测是否进行了分裂（假设action[2]是分裂信号）
        split_action = len(action) >= 3 and action[2] > 1.5
        
        if split_action:
            # 检查分裂是否在合适的时机
            current_cells = len(ps_current.clone) if hasattr(ps_current, 'clone') and isinstance(ps_current.clone, list) else 1
            food_nearby = len(ps_current.food) if hasattr(ps_current, 'food') else 0
            
            # 如果附近有足够食物且上次分裂间隔足够长
            steps_since_split = step_count - self.last_split_step
            if food_nearby >= 3 and steps_since_split >= 20:
                self.last_split_step = step_count
                return 1.0  # 战略分裂奖励
            else:
                return -0.2  # 不当分裂惩罚
        
        return 0.0
    
    def _calculate_food_density_reward(self, ps_current):
        """计算食物密度奖励（在食物密集区域活动）"""
        if not hasattr(ps_current, 'food'):
            return 0.0
        
        food_count = len(ps_current.food)
        
        # 根据视野内食物数量给予奖励
        if food_count >= 10:
            return 0.3
        elif food_count >= 5:
            return 0.1
        else:
            return 0.0
    
    def _calculate_basic_rewards(self, current_state):
        """计算基础奖励（生存、时间、死亡）"""
        rewards = {}
        
        # 生存奖励
        if not current_state or not hasattr(current_state, 'player_states') or not current_state.player_states:
            rewards['survival'] = 0.0
            rewards['death_penalty'] = 1.0  # 标记死亡
        else:
            rewards['survival'] = 1.0
            rewards['death_penalty'] = 0.0
        
        # 时间惩罚（鼓励快速决策）
        rewards['time_penalty'] = 1.0
        
        return rewards
    
    def _update_history(self, ps_current, reward_components, step_count):
        """更新历史状态追踪"""
        state_info = {
            'step': step_count,
            'score': ps_current.score,
            'reward_components': reward_components.copy(),
            'cells': len(ps_current.clone) if hasattr(ps_current, 'clone') and isinstance(ps_current.clone, list) else 1,
            'food_count': len(ps_current.food) if hasattr(ps_current, 'food') else 0
        }
        
        self.history.append(state_info)
        
        # 保持历史长度
        if len(self.history) > 100:
            self.history.pop(0)
        
        # 更新移动平均分数
        if len(self.history) >= self.moving_avg_window:
            recent_scores = [h['score'] for h in self.history[-self.moving_avg_window:]]
            self.moving_avg_score = sum(recent_scores) / len(recent_scores)
    
    def get_reward_analysis(self):
        """获取奖励分析报告"""
        if not self.history:
            return "暂无历史数据"
        
        latest = self.history[-1]
        
        analysis = f"""
奖励分析报告
===========
当前步数: {latest['step']}
当前分数: {latest['score']:.2f}
移动平均分数: {self.moving_avg_score:.2f}
连续增长步数: {self.consecutive_growth_steps}
食物消费次数: {self.food_eaten_count}
探索区域数: {len(self.visited_regions)}

最新奖励组件:
"""
        
        for component, value in latest['reward_components'].items():
            weighted_value = self.weights.get(component, 1.0) * value
            analysis += f"  {component}: {value:.4f} (权重后: {weighted_value:.4f})\n"
        
        return analysis

# 使用示例
def demo_enhanced_reward():
    """演示增强奖励系统"""
    print("🎯 增强奖励系统演示")
    print("=" * 50)
    
    calculator = EnhancedRewardCalculator()
    
    # 模拟一些游戏状态...
    # 这里可以添加具体的测试代码
    
    print("增强奖励系统已准备就绪！")
    print("\n特性:")
    print("✅ 非线性分数增长奖励")
    print("✅ 效率奖励（分数/移动比率）")
    print("✅ 探索新区域奖励")
    print("✅ 战略分裂奖励")
    print("✅ 食物密度奖励")
    print("✅ 连续增长加成")

if __name__ == "__main__":
    demo_enhanced_reward()
