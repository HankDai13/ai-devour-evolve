import sys
import os
from pathlib import Path
import numpy as np
import time
import json
import math
from collections import deque, defaultdict
from typing import Dict, Tuple, Set, Optional, List, Any
from dataclasses import dataclass

# 统一的路径设置
# 1. 将项目根目录添加到 sys.path，以解决模块导入问题 (e.g., from python.xxx)
current_dir = Path(__file__).parent
root_dir = current_dir.parent
if str(root_dir) not in sys.path:
    sys.path.insert(0, str(root_dir))

# 2. 添加 C++ 编译的模块路径
build_dir = root_dir / "build" / "Release"
if str(build_dir) not in sys.path:
    sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

import gobigger_env
from python.multi_agent_gobigger_gym_env import MultiAgentGoBiggerEnv

try:
    # 尝试导入stable-baselines3 (如果已安装)
    from stable_baselines3 import PPO
    from stable_baselines3.common.env_util import make_vec_env
    from stable_baselines3.common.vec_env import SubprocVecEnv, DummyVecEnv
    from stable_baselines3.common.callbacks import BaseCallback, CallbackList, EvalCallback
    from stable_baselines3.common.logger import configure
    from stable_baselines3.common.monitor import Monitor
    STABLE_BASELINES_AVAILABLE = True
    print("✅ 检测到 stable-baselines3，将使用专业RL算法")
except ImportError:
    STABLE_BASELINES_AVAILABLE = False
    print("⚠️  未检测到 stable-baselines3，将使用简单的随机策略演示")
    print("💡 安装命令: pip install stable-baselines3[extra]")

import gymnasium as gym

@dataclass
class ExplorationState:
    """探索状态追踪"""
    visited_cells: Set[Tuple[int, int]]
    cell_visit_count: Dict[Tuple[int, int], int]
    cell_size: int = 100

class MultiAgentRewardCalculator:
    """
    🔥 多智能体优化的奖励计算器
    
    实现多智能体协作与对抗的核心奖励：
    1. 队伍排名奖励
    2. 吞噬敌人奖励
    3. 智能分裂追击奖励
    4. 探索与协作奖励
    """
    
    def __init__(self, team_id: int, player_names: List[str], config: Optional[Dict] = None):
        self.team_id = team_id
        self.player_names = player_names
        self.config = config or {}
        
        # === 🔥 核心多智能体奖励权重配置 ===
        self.weights = {
            # 基础奖励
            'score_growth': 1.0,           # 分数增长基础奖励
            'survival': 0.01,              # 生存奖励
            'death_penalty': -10.0,        # 死亡惩罚
            
            # === 核心对抗奖励 ===
            'eat_opponent_bonus': 15.0,    # 🔥 吞噬敌人核心奖励
            'split_to_eat_bonus': 20.0,    # 🔥 分裂追击吞噬奖励
            'wasted_split_penalty': -5.0,  # 浪费分裂惩罚
            
            # === 团队排名奖励 ===
            'team_rank_1_bonus': 200.0,    # 🔥 队伍第一名奖励
            'team_rank_2_bonus': 50.0,     # 队伍第二名奖励
            'team_rank_last_penalty': -100.0, # 队伍最后一名惩罚

            # === 探索与协作 ===
            'exploration_bonus': 1.5,      # 新区域探索奖励
            'team_coverage_bonus': 2.0,    # 团队地图覆盖度奖励
        }
        
        # === 状态追踪 ===
        self.history = {name: deque(maxlen=100) for name in player_names}
        self.step_count = 0
        
        # === 探索状态追踪 ===
        self.exploration_states = {
            name: ExplorationState(
                visited_cells=set(),
                cell_visit_count=defaultdict(int),
                cell_size=self.config.get('exploration_cell_size', 100)
            ) for name in player_names
        }

        # Add missing attribute for split efficiency window
        self.split_efficiency_window = self.config.get('split_efficiency_window', 30)

        # === Split and eat event tracking ===
        self.split_trackers = {name: deque(maxlen=5) for name in player_names}
        self.eat_trackers = {name: deque(maxlen=20) for name in player_names}

    def calculate_rewards(self, obs: Dict, rewards: Dict, dones: Dict, infos: Dict) -> Dict[str, float]:
        """
        计算所有智能体的优化奖励
        """
        self.step_count += 1
        final_rewards = {}

        for player_name in self.player_names:
            if player_name in infos and player_name in obs:
                player_info = infos[player_name]
                player_obs = obs[player_name]
                
                # 获取上一状态
                last_state = self.history[player_name][-1] if len(self.history[player_name]) > 0 else None

                # 计算单个智能体的奖励
                total_reward, reward_components = self._calculate_individual_reward(player_name, player_info, last_state, player_obs['action'])
                final_rewards[player_name] = total_reward

                # 更新历史
                self.history[player_name].append(player_info)

        # 在回合结束时计算团队奖励
        if any(dones.values()):
            team_rewards = self._calculate_team_rank_reward(infos)
            for player_name in self.player_names:
                if player_name in final_rewards:
                    final_rewards[player_name] += team_rewards

        return final_rewards

    def _calculate_individual_reward(self, player_name: str, player_info, last_state, action) -> Tuple[float, Dict[str, float]]:
        """
        计算单个智能体的奖励
        """
        reward_components = {}
        
        # 1. 分数增长奖励 (基础)
        if last_state:
            score_growth = player_info['score'] - last_state['score']
            if score_growth > 0:
                reward_components['score_growth'] = score_growth * self.weights['score_growth']

        # 2. 吞噬敌人奖励
        # 假设 info['eaten_opponents'] 是一个列表，包含被该玩家吃掉的敌方信息
        if 'eaten_opponents' in player_info and player_info['eaten_opponents']:
            num_eaten = len(player_info['eaten_opponents'])
            reward_components['eat_opponent_bonus'] = num_eaten * self.weights['eat_opponent_bonus']
            # 记录吞噬事件，用于分裂追击判断
            self._log_eat_event(player_name, player_info['eaten_opponents'])

        # 3. 智能分裂追击奖励
        split_reward, split_penalty = self._calculate_split_reward(player_name, action, player_info, last_state)
        if split_reward > 0:
            reward_components['split_to_eat_bonus'] = split_reward
        if split_penalty < 0:
            reward_components['wasted_split_penalty'] = split_penalty

        # 4. 探索奖励
        reward_components['exploration_bonus'] = self._calculate_exploration_reward(player_name, player_info) * self.weights['exploration_bonus']

        # 5. 死亡惩罚
        if player_info.get('is_dead', False):
            reward_components['death_penalty'] = self.weights['death_penalty']

        total_reward = sum(reward_components.values())
        return total_reward, reward_components

    def _calculate_split_reward(self, player_name: str, action, player_info, last_state) -> Tuple[float, float]:
        """
        计算分裂奖励或惩罚
        action[2] > 0 表示执行了分裂
        """
        # 检查分裂动作
        is_splitting = action and len(action) > 2 and action[2] > 0
        if is_splitting:
            # 记录分裂事件
            self.split_trackers[player_name].append({
                'step': self.step_count,
                'score_before_split': last_state['score'] if last_state else 0,
                'num_balls_before_split': len(last_state['player']) if last_state else 1
            })

        # 检查最近的分裂事件是否带来了收益
        reward = 0
        penalty = 0
        tracker = self.split_trackers[player_name]
        if tracker:
            # 检查最近一次分裂
            last_split = tracker[-1]
            steps_since_split = self.step_count - last_split['step']

            # 在分裂后的一个窗口期内检查收益
            if 1 < steps_since_split < self.split_efficiency_window:
                # 检查此窗口期内是否有吞噬事件
                if any(eat['step'] > last_split['step'] for eat in self.eat_trackers[player_name]):
                    reward = self.weights['split_to_eat_bonus']
                    # 清除已奖励的事件，避免重复奖励
                    self.eat_trackers[player_name].clear()
                    tracker.pop()
            
            # 如果分裂后太久没有收益，则惩罚
            elif steps_since_split > self.split_efficiency_window:
                penalty = self.weights['wasted_split_penalty']
                tracker.pop() # 移除旧的追踪，避免重复惩罚

        return reward, penalty

    def _log_eat_event(self, player_name: str, eaten_opponents: List):
        self.eat_trackers[player_name].append({
            'step': self.step_count,
            'opponents': eaten_opponents
        })

    def _calculate_team_rank_reward(self, infos: Dict) -> float:
        """
        根据最终队伍排名计算奖励
        """
        if 'team_rank' in infos:
            team_rank = infos['team_rank']
            if self.team_id in team_rank:
                rank = team_rank[self.team_id]
                if rank == 1:
                    return self.weights['team_rank_1_bonus']
                elif rank == 2:
                    return self.weights['team_rank_2_bonus']
                else:
                    return self.weights['team_rank_last_penalty']
        return 0.0

    def _check_eaten_opponents(self, current_overlap, last_overlap) -> int:
        # 此函数的功能已被合并到对 info['eaten_opponents'] 的直接检查中
        return 0

    def _calculate_exploration_reward(self, player_name: str, player_info) -> float:
        """
        计算探索奖励
        """
        exploration_reward = 0
        player_pos = (player_info['player'][0][0], player_info['player'][0][1]) # 取第一个球的位置
        cell_size = self.exploration_states[player_name].cell_size
        cell = (int(player_pos[0] // cell_size), int(player_pos[1] // cell_size))

        if cell not in self.exploration_states[player_name].visited_cells:
            exploration_reward = 1.0
            self.exploration_states[player_name].visited_cells.add(cell)
        
        return exploration_reward

    def reset(self):
        """
        重置所有状态
        """
        self.history = {name: deque(maxlen=100) for name in self.player_names}
        self.step_count = 0
        self.exploration_states = {
            name: ExplorationState(
                visited_cells=set(),
                cell_visit_count=defaultdict(int),
                cell_size=self.config.get('exploration_cell_size', 100)
            ) for name in self.player_names
        }

        # Reset split and eat event tracking
        self.split_trackers = {name: deque(maxlen=5) for name in self.player_names}
        self.eat_trackers = {name: deque(maxlen=20) for name in self.player_names}

class MultiAgentRewardCallback(BaseCallback):
    """
    Custom callback for multi-agent reward calculation during training.
    """
    def __init__(self, reward_calculator: MultiAgentRewardCalculator, verbose=0):
        super(MultiAgentRewardCallback, self).__init__(verbose)
        self.reward_calculator = reward_calculator

    def _on_step(self) -> bool:
        """
        Called after each environment step.
        """
        # For now, this callback is mainly for logging
        # The actual reward calculation is handled in the environment wrapper
        return True


def make_env(env_config, rank):
    """
    Helper function to create and wrap environment.
    """
    def _init():
        env = MultiAgentGoBiggerEnv(env_config)
        env = Monitor(env, f"{env_config['log_path']}/{rank}")
        return env
    return _init

from stable_baselines3.common.vec_env import VecEnv

class MultiAgentParallelWrapper(VecEnv):
    """
    Wrapper that converts multi-agent environment to stable-baselines3 VecEnv format,
    supporting parameter sharing for multi-agent training.
    """
    def __init__(self, env, rl_player_names):
        self.env = env
        self.rl_player_names = rl_player_names
        self.num_agents = len(rl_player_names)
        
        # 从第一个RL智能体获取 observation 和 action space
        # 在参数共享的设定下，所有智能体的 space 都是一样的
        agent_obs_space = self.env.observation_space[self.rl_player_names[0]]
        agent_action_space = self.env.action_space[self.rl_player_names[0]]

        super().__init__(self.num_agents, agent_obs_space, agent_action_space)

    def reset(self):
        obs_dict = self.env.reset()
        # 将观察字典转换为一个 numpy 数组
        return self._obs_dict_to_array(obs_dict)

    def step_async(self, actions):
        # 将PPO模型输出的动作数组，转换为环境需要的字典格式
        action_dict = {name: actions[i] for i, name in enumerate(self.rl_player_names)}
        self.actions_to_step = action_dict

    def step_wait(self):
        obs, rewards, dones, infos = self.env.step(self.actions_to_step)
        
        # 将输出的字典转换为数组
        obs_arr = self._obs_dict_to_array(obs)
        rewards_arr = np.array([rewards.get(name, 0) for name in self.rl_player_names], dtype=np.float32)
        dones_arr = np.array([dones.get(name, False) for name in self.rl_player_names], dtype=np.bool_)
        
        # 如果所有智能体都完成了，或者游戏结束了，我们需要重置
        if dones.get("__all__", False):
            obs_arr = self.reset()

        return obs_arr, rewards_arr, dones_arr, [infos] * self.num_agents

    def close(self):
        self.env.close()

    def get_attr(self, attr_name, indices=None):
        return getattr(self.env, attr_name)

    def set_attr(self, attr_name, value, indices=None):
        return setattr(self.env, attr_name, value)

    def env_is_wrapped(self, wrapper_class, indices=None):
        return [isinstance(self.env, wrapper_class)] * self.num_agents

    def env_method(self, method_name, *method_args, indices=None, **method_kwargs):
        return [getattr(self.env, method_name)(*method_args, **method_kwargs)] * self.num_agents

    def _obs_dict_to_array(self, obs_dict):
        # 确保我们总是以固定的顺序从字典中提取观察值
        return np.stack([obs_dict.get(name, np.zeros(self.observation_space.shape)) for name in self.rl_player_names])

def train_multi_agent(
    total_timesteps=2_000_000,
    num_rl_agents=3, # 我们训练的RL智能体数量
    num_food_hunters=3,
    num_aggressive_bots=3,
    env_server_host='127.0.0.1',
    env_server_port=10086,
    model_save_path='models/multi_agent_ppo_shared',
    log_path='logs/multi_agent_log_shared',
    learning_rate=3e-4,
    batch_size=2048, # 对于多智能体，可以适当增大
    n_steps=4096, # 每个智能体在更新前收集的步数
    gamma=0.99,
    ent_coef=0.01,
):
    """
    Multi-agent training using parameter sharing PPO.
    """
    if not STABLE_BASELINES_AVAILABLE:
        print("🛑 stable-baselines3 未安装，无法进行训练。")
        return

    # === 1. 环境设置 ===
    rl_player_names = [f'rl_player_{i}' for i in range(num_rl_agents)]
    bot_player_names = [f'food_hunter_{i}' for i in range(num_food_hunters)] + \
                       [f'aggressive_bot_{i}' for i in range(num_aggressive_bots)]

    env_config = {
        'player_num_per_team': num_rl_agents, # RL 队伍的玩家数
        'team_num': 2,
        'match_time': 600,
        'map_width': 1000,
        'map_height': 1000,
        'gamemode': 't2', # 2队模式
        'reward_config': { 'score_reward_weight': 1 }, # 基础奖励
        'server_host': env_server_host,
        'server_port': env_server_port,
        'team_player_names': {0: rl_player_names, 1: bot_player_names},
        # 在多智能体环境中，我们需要一种方式来指定不同bot的策略
        # 这需要在 multi_agent_gobigger_gym_env.py 中支持
        # 暂时我们假设环境能处理混合类型的bot
        'ai_opponent_type': 'mixed', 
        'rl_player_names': rl_player_names,
        'log_path': log_path,
    }

    # 创建并包装环境
    raw_env = MultiAgentGoBiggerEnv(env_config)
    vec_env = MultiAgentParallelWrapper(raw_env, rl_player_names)

    # === 2. 模型定义 (参数共享) ===
    model = PPO(
        'MlpPolicy', 
        vec_env,
        learning_rate=learning_rate,
        n_steps=n_steps // vec_env.num_envs, # n_steps 是总的，需要除以agent数量
        batch_size=batch_size,
        gamma=gamma,
        ent_coef=ent_coef,
        verbose=1,
        tensorboard_log=log_path
    )

    # === 3. 训练 ===
    print("🚀 开始多智能体 (参数共享) 训练...")
    print(f"🤖 RL 智能体: {num_rl_agents}")
    print(f"⚔️ 对手: {num_food_hunters} 食物猎手, {num_aggressive_bots} 攻击性AI")
    print(f"💾 模型将保存至: {model_save_path}")
    print(f"📊 日志将记录于: {log_path}")

    # 设置回调来定期保存模型
    from stable_baselines3.common.callbacks import CheckpointCallback
    checkpoint_callback = CheckpointCallback(save_freq=25000, save_path=model_save_path, name_prefix='rl_model')

    model.learn(
        total_timesteps=total_timesteps,
        tb_log_name="ppo_gobigger_multi_shared",
        callback=checkpoint_callback
    )

    model.save(f"{model_save_path}_final")
    print(f"✅ 训练完成，最终模型已保存至 {model_save_path}_final")

    vec_env.close()

if __name__ == '__main__':
    train_multi_agent()
