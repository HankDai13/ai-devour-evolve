#!/usr/bin/env python3
"""
GoBigger Gymnasium Environment Wrapper
将C++核心引擎包装成标准的Gymnasium环境
"""
import sys
import os
from pathlib import Path
import numpy as np

# 路径设置：定位到项目根目录
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

import gobigger_env

try:
    import gymnasium as gym
    from gymnasium import spaces
    GYMNASIUM_AVAILABLE = True
except ImportError:
    try:
        import gym
        from gym import spaces
        GYMNASIUM_AVAILABLE = False
    except ImportError:
        print("❌ 需要安装 gymnasium 或 gym")
        raise

class GoBiggerEnv(gym.Env if GYMNASIUM_AVAILABLE else gym.Env):
    """
    GoBigger环境的Gymnasium风格包装器
    提供标准的reset()、step()、render()接口
    """
    
    def __init__(self, config=None):
        """初始化环境"""
        super().__init__()
        
        self.engine = gobigger_env.GameEngine()
        self.config = config or {}
        
        # 定义动作空间和观察空间
        self.action_space = spaces.Box(
            low=np.array([-1.0, -1.0, 0], dtype=np.float32),
            high=np.array([1.0, 1.0, 2], dtype=np.float32),
            dtype=np.float32
        )
        
        self.observation_space = spaces.Box(
            low=-np.inf,
            high=np.inf,
            shape=(400,),
            dtype=np.float32
        )
        
        # 动作空间定义（兼容性保留）
        self.action_space_low = np.array([-1.0, -1.0, 0], dtype=np.float32)
        self.action_space_high = np.array([1.0, 1.0, 2], dtype=np.float32)
        
        # 环境状态
        self.current_obs = None
        self.episode_step = 0
        self.max_episode_steps = self.config.get('max_episode_steps', 1000)
        
    def reset(self, seed=None):
        """重置环境"""
        self.episode_step = 0
        self.current_obs = self.engine.reset()
        return self._extract_features(), {}
    
    def step(self, action):
        """执行一步动作"""
        # 确保动作格式正确
        if isinstance(action, (list, tuple, np.ndarray)):
            if len(action) >= 3:
                cpp_action = gobigger_env.Action(float(action[0]), float(action[1]), int(action[2]))
            else:
                cpp_action = gobigger_env.Action(0.0, 0.0, 0)
        else:
            cpp_action = action
        
        # 执行动作
        self.current_obs = self.engine.step(cpp_action)
        self.episode_step += 1
        
        # 计算奖励和终止条件
        reward = self._calculate_reward()
        terminated = self.engine.is_done()
        truncated = self.episode_step >= self.max_episode_steps
        
        return self._extract_features(), reward, terminated, truncated, {}
    
    def _extract_features(self):
        """从C++观察中提取特征向量"""
        if not self.current_obs or len(self.current_obs.player_states) == 0:
            return np.zeros(400, dtype=np.float32)  # 默认特征大小
        
        # 获取第一个玩家的状态
        ps = list(self.current_obs.player_states.values())[0]
        
        features = []
        
        # 全局特征 (10维)
        gs = self.current_obs.global_state
        features.extend([
            gs.total_frame / 1000.0,  # 归一化帧数
            len(gs.leaderboard),      # 队伍数量
            ps.score / 10000.0,       # 归一化分数
            float(ps.can_eject),      # 能否吐球
            float(ps.can_split),      # 能否分裂
        ])
        features.extend([0.0] * 5)  # 预留5个全局特征
        
        # 视野矩形 (4维)
        features.extend(ps.rectangle)
        
        # 扁平化对象特征
        # 食物: 50个 × 4维 = 200维
        for food in ps.food:
            features.extend(food)
        
        # 荆棘: 20个 × 6维 = 120维  
        for thorns in ps.thorns:
            features.extend(thorns)
        
        # 孢子: 10个 × 6维 = 60维
        for spore in ps.spore:
            features.extend(spore)
        
        # 截断或填充到固定长度
        target_length = 400  # 10 + 4 + 200 + 120 + 60 + 预留
        if len(features) > target_length:
            features = features[:target_length]
        elif len(features) < target_length:
            features.extend([0.0] * (target_length - len(features)))
        
        return np.array(features, dtype=np.float32)
    
    def _calculate_reward(self):
        """计算奖励"""
        if not self.current_obs or len(self.current_obs.player_states) == 0:
            return 0.0
        
        ps = list(self.current_obs.player_states.values())[0]
        
        # 基于分数的奖励
        score_reward = ps.score / 1000.0
        
        # 生存奖励
        survival_reward = 1.0 if not self.engine.is_done() else 0.0
        
        return score_reward * 0.1 + survival_reward * 0.01
    
    def render(self, mode='human'):
        """渲染环境（可选实现）"""
        if mode == 'human':
            print(f"Frame: {self.current_obs.global_state.total_frame if self.current_obs else 0}")
            if self.current_obs and len(self.current_obs.player_states) > 0:
                ps = list(self.current_obs.player_states.values())[0]
                print(f"Score: {ps.score}, Can eject: {ps.can_eject}, Can split: {ps.can_split}")
    
    def close(self):
        """清理资源"""
        pass

def demo_gymnasium_usage():
    """演示标准Gymnasium使用方式"""
    print("🎮 GoBigger Gymnasium环境演示")
    
    # 创建环境
    env = GoBiggerEnv({'max_episode_steps': 100})
    
    # 重置环境
    obs, info = env.reset()
    print(f"✅ 环境重置，观察维度: {obs.shape}")
    
    # 运行几步
    total_reward = 0
    for step in range(10):
        # 随机动作
        action = np.random.uniform(env.action_space_low, env.action_space_high)
        action[2] = int(action[2])  # 动作类型为整数
        
        obs, reward, terminated, truncated, info = env.step(action)
        total_reward += reward
        
        print(f"步骤 {step+1}: action={action}, reward={reward:.4f}, done={terminated or truncated}")
        
        if terminated or truncated:
            print("🏁 回合结束")
            break
    
    print(f"✅ 总奖励: {total_reward:.4f}")
    env.close()

if __name__ == "__main__":
    demo_gymnasium_usage()
