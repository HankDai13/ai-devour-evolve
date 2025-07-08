#!/usr/bin/env python3
"""
Multi-Agent GoBigger Gymnasium Environment
支持RL智能体与传统AI机器人混合训练的多智能体环境
"""
import sys
import os
import time
from pathlib import Path
import numpy as np
from typing import Dict, Any, List, Tuple, Optional

# 智能路径搜索：寻找有效的构建目录
root_dir = Path(__file__).parent.parent
build_dir_candidates = [
    root_dir / "build" / "Release",
    root_dir / "build" / "Debug", 
    root_dir / "build",
    root_dir / "build-msvc" / "Release",
    root_dir / "build-msvc" / "Debug",
]

build_dir_found = None
for build_dir in build_dir_candidates:
    if build_dir.exists() and any(build_dir.iterdir()):
        sys.path.insert(0, str(build_dir))
        os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"
        build_dir_found = build_dir
        print(f"✅ 找到构建目录: {build_dir}")
        break

if not build_dir_found:
    raise ImportError("❌ 未找到有效的构建目录。请确保项目已编译。")

try:
    import gobigger_env
    print("✅ 成功导入 gobigger_env 模块")
except ImportError as e:
    print(f"❌ 导入 gobigger_env 失败: {e}")
    print(f"搜索路径: {build_dir_found}")
    raise

try:
    import gymnasium as gym
    from gymnasium import spaces
    GYMNASIUM_AVAILABLE = True
    print("✅ 使用 gymnasium 库")
except ImportError:
    try:
        import gym
        from gym import spaces
        GYMNASIUM_AVAILABLE = False
        print("✅ 使用 gym 库")
    except ImportError:
        print("❌ 需要安装 gymnasium 或 gym")
        raise

class MultiAgentGoBiggerEnv(gym.Env if GYMNASIUM_AVAILABLE else gym.Env):
    """
    多智能体GoBigger环境
    
    这个环境支持一个RL智能体与多个传统AI策略机器人对战。
    传统AI使用您在src_new中已实现的策略（FOOD_HUNTER, AGGRESSIVE, RANDOM），
    而RL智能体通过标准的Gymnasium接口进行训练。
    
    特点：
    - 支持团队排名作为奖励信号
    - 传统AI提供稳定的对手
    - 多智能体环境增加训练复杂度
    """
    
    def __init__(self, config: Optional[Dict[str, Any]] = None):
        """初始化多智能体环境"""
        super().__init__()
        
        # 环境配置
        self.config = config or {}
        
        # 创建C++多智能体引擎配置
        engine_config = gobigger_env.MultiAgentConfig()
        engine_config.maxFoodCount = self.config.get('max_food_count', 3000)
        engine_config.initFoodCount = self.config.get('init_food_count', 2500)
        engine_config.maxThornsCount = self.config.get('max_thorns_count', 12)
        engine_config.initThornsCount = self.config.get('init_thorns_count', 9)
        engine_config.maxFrames = self.config.get('max_frames', 3000)
        engine_config.aiOpponentCount = self.config.get('ai_opponent_count', 3)
        engine_config.gameUpdateInterval = self.config.get('update_interval', 16)
        
        # 创建C++引擎
        self.engine = gobigger_env.MultiAgentGameEngine(engine_config)
        
        # 定义RL智能体的动作和观察空间
        # 动作: [move_x, move_y, action_type]
        # move_x, move_y: [-1.0, 1.0] 移动方向
        # action_type: 0=移动, 1=分裂, 2=喷射
        self.action_space = spaces.Box(
            low=np.array([-1.0, -1.0, 0], dtype=np.float32),
            high=np.array([1.0, 1.0, 2], dtype=np.float32),
            dtype=np.float32
        )
        
        # 观察空间：特征向量 + 团队排名信息
        self.observation_space_shape = (450,)  # 增加空间以包含排名信息
        self.observation_space = spaces.Box(
            low=-np.inf,
            high=np.inf,
            shape=self.observation_space_shape,
            dtype=np.float32
        )
        
        # 环境状态
        self.current_obs = None
        self.episode_step = 0
        self.max_episode_steps = self.config.get('max_episode_steps', 3000)
        
        # 奖励追踪
        self.last_score = 0.0
        self.initial_score = 0.0
        self.last_team_rank = 1
        self.episode_reward = 0.0
        
        # 调试模式
        self.debug_mode = self.config.get('debug_mode', False)
        
        print(f"🎮 多智能体环境初始化完成")
        print(f"   AI对手数量: {engine_config.aiOpponentCount}")
        print(f"   最大帧数: {engine_config.maxFrames}")
        print(f"   观察空间: {self.observation_space.shape}")
        print(f"   动作空间: {self.action_space.shape}")
    
    def reset(self, seed: Optional[int] = None, options: Optional[Dict] = None):
        """重置环境"""
        if seed is not None:
            np.random.seed(seed)
        
        # 重置C++引擎
        self.current_obs = self.engine.reset()
        self.episode_step = 0
        self.episode_reward = 0.0
        
        # 初始化奖励追踪
        reward_info = self.engine.get_reward_info()
        self.last_score = reward_info.get('score', 0.0)
        self.initial_score = self.last_score
        self.last_team_rank = reward_info.get('team_rank', 1)
        
        if self.debug_mode:
            print(f"🔄 环境重置 - 初始分数: {self.initial_score}, 初始排名: {self.last_team_rank}")
        
        # 兼容不同版本的gym
        if GYMNASIUM_AVAILABLE:
            return self._extract_features(), {}
        else:
            return self._extract_features()
    
    def step(self, action):
        """执行一步动作"""
        # 确保动作格式正确并进行裁剪
        if isinstance(action, (list, tuple, np.ndarray)):
            if len(action) >= 3:
                move_x = float(np.clip(action[0], -1.0, 1.0))
                move_y = float(np.clip(action[1], -1.0, 1.0))
                action_type = int(np.round(np.clip(action[2], 0, 2)))
                rl_action = [move_x, move_y, action_type]
            else:
                rl_action = [0.0, 0.0, 0]
        else:
            rl_action = [0.0, 0.0, 0]
        
        # 构造多智能体动作字典
        actions_dict = {"rl_agent": rl_action}
        
        # 执行动作
        self.current_obs = self.engine.step(actions_dict)
        self.episode_step += 1
        
        # 计算奖励
        reward = self._calculate_multi_agent_reward()
        
        # 检查终止条件
        terminated = self.engine.is_done()
        truncated = self.episode_step >= self.max_episode_steps
        
        # 累积奖励
        self.episode_reward += reward
        
        # 准备信息字典
        info = self._prepare_info_dict(terminated or truncated)
        
        # 兼容不同版本的gym
        if GYMNASIUM_AVAILABLE:
            return self._extract_features(), reward, terminated, truncated, info
        else:
            return self._extract_features(), reward, terminated or truncated, info
    
    def _extract_features(self) -> np.ndarray:
        """从多智能体观察中提取特征向量"""
        if not self.current_obs:
            return np.zeros(self.observation_space_shape, dtype=np.float32)
        
        features = []
        
        # 1. 全局状态特征 (10维)
        global_state = self.current_obs.get('global_state', {})
        features.extend([
            global_state.get('frame', 0) / self.max_episode_steps,
            global_state.get('total_players', 0) / 10.0,
            global_state.get('food_count', 0) / 3000.0,
            global_state.get('thorns_count', 0) / 15.0,
        ])
        features.extend([0.0] * 6)  # 预留6个全局特征
        
        # 2. RL智能体状态特征 (20维)
        rl_obs = self.current_obs.get('rl_agent', {})
        if rl_obs:
            # 位置和基础属性
            pos = rl_obs.get('position', (0, 0))
            vel = rl_obs.get('velocity', (0, 0))
            features.extend([
                pos[0] / 2000.0,  # 归一化位置
                pos[1] / 2000.0,
                rl_obs.get('radius', 0) / 100.0,
                rl_obs.get('score', 0) / 10000.0,
                vel[0] / 1000.0,  # 归一化速度
                vel[1] / 1000.0,
                float(rl_obs.get('can_split', False)),
                float(rl_obs.get('can_eject', False)),
            ])
            features.extend([0.0] * 12)  # 预留12个RL特征
        else:
            features.extend([0.0] * 20)  # RL智能体死亡时填0
        
        # 3. 团队排名特征 (20维)
        team_ranking = global_state.get('team_ranking', [])
        ranking_features = [0.0] * 20
        
        for i, team_info in enumerate(team_ranking[:5]):  # 最多5个队伍
            base_idx = i * 4
            if base_idx + 3 < len(ranking_features):
                ranking_features[base_idx] = team_info.get('team_id', 0) / 10.0
                ranking_features[base_idx + 1] = team_info.get('score', 0) / 10000.0
                ranking_features[base_idx + 2] = team_info.get('rank', 1) / 5.0
                ranking_features[base_idx + 3] = 1.0 if team_info.get('team_id', -1) == 0 else 0.0  # 是否是RL队伍
        
        features.extend(ranking_features)
        
        # 4. 视野内食物特征 (200维: 50个食物 × 4维)
        nearby_food = rl_obs.get('nearby_food', [])
        food_features = []
        for i in range(50):
            if i < len(nearby_food):
                food = nearby_food[i]
                food_features.extend([
                    food[0] / 2000.0,  # x
                    food[1] / 2000.0,  # y
                    food[2] / 10.0,    # radius
                    food[3] / 100.0,   # score
                ])
            else:
                food_features.extend([0.0, 0.0, 0.0, 0.0])
        features.extend(food_features)
        
        # 5. 视野内其他玩家特征 (200维: 20个玩家 × 10维)
        nearby_players = rl_obs.get('nearby_players', [])
        player_features = []
        for i in range(20):
            if i < len(nearby_players):
                player = nearby_players[i]
                player_features.extend([
                    player[0] / 2000.0,  # x
                    player[1] / 2000.0,  # y
                    player[2] / 100.0,   # radius
                    player[3] / 10000.0, # score
                    player[4] / 10.0,    # team_id
                    player[5] / 10.0,    # player_id
                ])
                player_features.extend([0.0] * 4)  # 预留4维
            else:
                player_features.extend([0.0] * 10)
        features.extend(player_features)
        
        # 截断或填充到固定长度
        target_length = self.observation_space_shape[0]
        if len(features) > target_length:
            features = features[:target_length]
        elif len(features) < target_length:
            features.extend([0.0] * (target_length - len(features)))
        
        return np.array(features, dtype=np.float32)
    
    def _calculate_multi_agent_reward(self) -> float:
        """计算多智能体奖励"""
        reward_info = self.engine.get_reward_info()
        
        # 1. 分数增长奖励
        current_score = reward_info.get('score', 0.0)
        score_delta = current_score - self.last_score
        score_reward = score_delta / 100.0
        
        # 2. 团队排名奖励（核心创新）
        current_rank = reward_info.get('team_rank', 1)
        total_teams = reward_info.get('total_teams', 1)
        
        # 排名奖励：排名越高奖励越大
        rank_reward = (total_teams - current_rank + 1) / total_teams * 0.5
        
        # 排名变化奖励：排名提升给予额外奖励
        rank_change_reward = 0.0
        if current_rank < self.last_team_rank:
            rank_change_reward = 2.0  # 排名提升奖励
        elif current_rank > self.last_team_rank:
            rank_change_reward = -1.0  # 排名下降惩罚
        
        # 3. 生存奖励
        is_alive = reward_info.get('alive', False)
        survival_reward = 0.01 if is_alive else -10.0
        
        # 4. 时间惩罚（鼓励快速决策）
        time_penalty = -0.001
        
        # 5. 高级动作奖励（保持原有的Split/Eject奖励机制）
        advanced_action_reward = 0.0
        # 这里可以根据action类型添加奖励，但需要从C++端传递更多信息
        
        # 总奖励
        total_reward = (score_reward + rank_reward + rank_change_reward + 
                       survival_reward + time_penalty + advanced_action_reward)
        
        # 更新状态
        self.last_score = current_score
        self.last_team_rank = current_rank
        
        if self.debug_mode and self.episode_step % 100 == 0:
            print(f"🎯 Step {self.episode_step}: "
                  f"Score={current_score:.1f}(Δ{score_delta:+.1f}), "
                  f"Rank={current_rank}/{total_teams}, "
                  f"Reward={total_reward:.3f}")
        
        return total_reward
    
    def _prepare_info_dict(self, episode_done: bool) -> Dict[str, Any]:
        """准备信息字典"""
        info = {}
        
        if episode_done:
            reward_info = self.engine.get_reward_info()
            final_score = reward_info.get('score', 0.0)
            final_rank = reward_info.get('team_rank', 1)
            total_teams = reward_info.get('total_teams', 1)
            
            # Monitor标准格式
            info['episode'] = {
                'r': self.episode_reward,
                'l': self.episode_step,
                't': time.time()
            }
            
            # 多智能体特定信息
            info['final_score'] = final_score
            info['score_delta'] = final_score - self.initial_score
            info['final_rank'] = final_rank
            info['total_teams'] = total_teams
            info['rank_progress'] = (total_teams - final_rank + 1) / total_teams
            
            # AI对手状态
            ai_states = self.current_obs.get('ai_states', {})
            info['ai_opponents_alive'] = len([s for s in ai_states.values() 
                                            if s.get('alive_balls_count', 0) > 0])
            
            if self.debug_mode:
                print(f"🏁 Episode结束:")
                print(f"   最终分数: {final_score:.1f} (增长: {final_score - self.initial_score:+.1f})")
                print(f"   最终排名: {final_rank}/{total_teams}")
                print(f"   Episode奖励: {self.episode_reward:.3f}")
                print(f"   存活AI对手: {info['ai_opponents_alive']}")
        
        return info
    
    def render(self, mode: str = 'human'):
        """渲染环境"""
        if mode == 'human' and self.current_obs:
            global_state = self.current_obs.get('global_state', {})
            rl_obs = self.current_obs.get('rl_agent', {})
            
            print(f"Frame: {global_state.get('frame', 0)}")
            if rl_obs:
                print(f"RL Agent - Score: {rl_obs.get('score', 0):.1f}, "
                      f"Pos: ({rl_obs.get('position', (0, 0))[0]:.0f}, "
                      f"{rl_obs.get('position', (0, 0))[1]:.0f})")
            
            # 显示团队排名
            team_ranking = global_state.get('team_ranking', [])
            print("Team Ranking:")
            for i, team in enumerate(team_ranking):
                marker = "👑" if team.get('team_id') == 0 else "🤖"
                print(f"  {i+1}. {marker} Team {team.get('team_id', 0)}: {team.get('score', 0):.1f}")
    
    def close(self):
        """清理资源"""
        print("🔚 多智能体环境关闭")

def demo_multi_agent_usage():
    """演示多智能体环境使用"""
    print("\n" + "="*60)
    print("🎮 多智能体 GoBigger 环境演示")
    print("="*60 + "\n")
    
    # 创建环境
    config = {
        'max_episode_steps': 200,
        'ai_opponent_count': 3,
        'debug_mode': True,
        'max_frames': 1000
    }
    env = MultiAgentGoBiggerEnv(config)
    
    # 检查环境（如果安装了stable-baselines3）
    try:
        from stable_baselines3.common.env_checker import check_env
        print("🔍 使用 stable-baselines3 检查环境...")
        check_env(env)
        print("✅ 环境检查通过！\n")
    except ImportError:
        print("⚠️ 未安装 stable-baselines3，跳过环境检查\n")
    
    # 重置环境
    if GYMNASIUM_AVAILABLE:
        obs, info = env.reset()
    else:
        obs = env.reset()
        info = {}
    
    print(f"🏁 环境重置完成")
    print(f"   观察空间: {env.observation_space}")
    print(f"   动作空间: {env.action_space}")
    print(f"   观察维度: {obs.shape}")
    print()
    
    # 运行几步
    total_reward = 0
    for step in range(10):
        # 随机动作
        action = env.action_space.sample()
        
        if GYMNASIUM_AVAILABLE:
            obs, reward, terminated, truncated, info = env.step(action)
            done = terminated or truncated
        else:
            obs, reward, done, info = env.step(action)
            terminated, truncated = done, False
        
        total_reward += reward
        
        print(f"步骤 {step+1:02d}: "
              f"动作=[{action[0]:.2f}, {action[1]:.2f}, {int(np.round(action[2]))}], "
              f"奖励={reward:.4f}, "
              f"累计奖励={total_reward:.4f}, "
              f"结束={done}")
        
        if step % 3 == 0:  # 每3步渲染一次
            env.render()
            print()
        
        if done:
            print(f"\n🏁 回合结束 (第{step+1}步)")
            if 'final_rank' in info:
                print(f"   最终排名: {info['final_rank']}/{info.get('total_teams', 1)}")
            break
    
    print(f"\n✅ 演示完成！")
    print(f"   最终累计奖励: {total_reward:.4f}")
    print(f"   平均步骤奖励: {total_reward/(step+1):.4f}")
    
    env.close()

if __name__ == "__main__":
    demo_multi_agent_usage()
