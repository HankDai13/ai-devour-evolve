#!/usr/bin/env python3
"""
GoBigger Gymnasium Environment Wrapper
将C++核心引擎包装成标准的Gymnasium环境
"""
import sys
import os
import time
from pathlib import Path
import numpy as np

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
        
        # 观察空间大小需要与特征提取保持一致
        self.observation_space_shape = (400,)
        self.observation_space = spaces.Box(
            low=-np.inf,
            high=np.inf,
            shape=self.observation_space_shape,
            dtype=np.float32
        )
        
        # 动作空间定义（兼容性保留）
        self.action_space_low = np.array([-1.0, -1.0, 0], dtype=np.float32)
        self.action_space_high = np.array([1.0, 1.0, 2], dtype=np.float32)
        
        # 环境状态
        self.current_obs = None
        self.episode_step = 0
        self.max_episode_steps = self.config.get('max_episode_steps', 2000)  # 增加默认步数
        
        # 奖励函数优化：追踪分数变化
        self.last_score = 0.0
        self.initial_score = 0.0  # 记录初始分数
        self.final_score = 0.0    # 记录最终分数
        self.prev_observation = None
        
    def reset(self, seed=None, options=None):
        """重置环境"""
        if seed is not None:
            # 设置随机种子（如果引擎支持）
            np.random.seed(seed)
        
        self.episode_step = 0
        self.episode_reward = 0.0  # 重置episode累积奖励
        self.current_obs = self.engine.reset()
        
        # 初始化奖励函数状态
        if self.current_obs and self.current_obs.player_states:
            ps = list(self.current_obs.player_states.values())[0]
            self.last_score = ps.score
            self.initial_score = ps.score  # 记录初始分数
        else:
            self.last_score = 0.0
            self.initial_score = 0.0

        self.final_score = 0.0  # 重置最终分数
        self.prev_observation = self.current_obs
        
        # 重置增强奖励计算器（如果启用）
        if hasattr(self, 'enhanced_reward_calculator'):
            self.enhanced_reward_calculator.reset()
            self.reward_components_history = []
        
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
                # 裁剪动作到有效范围
                move_x = float(np.clip(action[0], -1.0, 1.0))
                move_y = float(np.clip(action[1], -1.0, 1.0))
                action_type = int(np.round(np.clip(action[2], 0, 2)))
                cpp_action = gobigger_env.Action(move_x, move_y, action_type)
            else:
                cpp_action = gobigger_env.Action(0.0, 0.0, 0)
        else:
            cpp_action = action
        
        # 执行动作
        self.prev_observation = self.current_obs
        self.current_obs = self.engine.step(cpp_action)
        self.episode_step += 1
        
        # 计算奖励和终止条件（传递原始action用于增强奖励计算）
        reward = self._calculate_reward(action)
        terminated = self.engine.is_done()
        truncated = self.episode_step >= self.max_episode_steps
        
        # 准备info字典
        info = {}
        
        # 累积episode总奖励（用于Monitor统计）
        if not hasattr(self, 'episode_reward'):
            self.episode_reward = 0.0
        self.episode_reward += reward
        
        # 如果episode结束，记录最终分数
        if terminated or truncated:
            if self.current_obs and self.current_obs.player_states:
                ps = list(self.current_obs.player_states.values())[0]
                self.final_score = ps.score
            else:
                self.final_score = self.last_score
            
            # Monitor期望的标准键
            info['episode'] = {
                'r': self.episode_reward,                     # episode总奖励（累积的step奖励）
                'l': self.episode_step,                       # episode长度
                't': time.time()                              # 时间戳
            }
            
            # 额外的统计信息
            info['final_score'] = self.final_score
            info['score_delta'] = self.final_score - self.initial_score
            info['episode_length'] = self.episode_step
        
        # 兼容不同版本的gym
        if GYMNASIUM_AVAILABLE:
            return self._extract_features(), reward, terminated, truncated, info
        else:
            return self._extract_features(), reward, terminated or truncated, info
    
    def _extract_features(self):
        """从C++观察中提取特征向量（优化版）"""
        if not self.current_obs or not self.current_obs.player_states:
            return np.zeros(self.observation_space_shape, dtype=np.float32)
        
        # 获取第一个玩家的状态
        ps = list(self.current_obs.player_states.values())[0]
        
        features = []
        
        # 全局特征 (10维) - 改进归一化
        gs = self.current_obs.global_state
        features.extend([
            gs.total_frame / self.max_episode_steps,  # 归一化帧数
            len(gs.leaderboard) / 10.0,               # 归一化队伍数量
            ps.score / 10000.0,                       # 归一化分数
            float(ps.can_eject),                      # 能否吐球
            float(ps.can_split),                      # 能否分裂
        ])
        features.extend([0.0] * 5)  # 预留5个全局特征
        
        # 视野矩形 (4维) - 改进归一化
        features.extend([coord / 2000.0 for coord in ps.rectangle])
        
        # 扁平化对象特征 - 改进归一化和数据处理
        # 食物: 50个 × 4维 = 200维
        food_count = 0
        for food in ps.food:
            if food_count >= 50:
                break
            # 归一化坐标和半径
            features.extend([
                food[0] / 2000.0,   # x坐标归一化
                food[1] / 2000.0,   # y坐标归一化  
                food[2] / 10.0,     # 半径归一化
                2.0                 # 类型标识
            ])
            food_count += 1
        # 填充剩余的食物槽位
        features.extend([0.0] * ((50 - food_count) * 4))
        
        # 荆棘: 20个 × 4维 = 80维
        thorns_count = 0
        for thorns in ps.thorns:
            if thorns_count >= 20:
                break
            features.extend([
                thorns[0] / 2000.0,  # x坐标归一化
                thorns[1] / 2000.0,  # y坐标归一化
                thorns[2] / 100.0,   # 半径归一化
                3.0                  # 类型标识
            ])
            thorns_count += 1
        # 填充剩余的荆棘槽位
        features.extend([0.0] * ((20 - thorns_count) * 4))
        
        # 孢子: 10个 × 4维 = 40维
        spore_count = 0
        for spore in ps.spore:
            if spore_count >= 10:
                break
            features.extend([
                spore[0] / 2000.0,   # x坐标归一化
                spore[1] / 2000.0,   # y坐标归一化
                spore[2] / 20.0,     # 半径归一化
                4.0                  # 类型标识
            ])
            spore_count += 1
        # 填充剩余的孢子槽位
        features.extend([0.0] * ((10 - spore_count) * 4))
        
        # 截断或填充到固定长度
        # 总计: 10 + 4 + 200 + 80 + 40 = 334维，填充到400维
        target_length = self.observation_space_shape[0]
        if len(features) > target_length:
            features = features[:target_length]
        elif len(features) < target_length:
            features.extend([0.0] * (target_length - len(features)))
        
        return np.array(features, dtype=np.float32)
    
    def _calculate_reward(self, action=None):
        """
        计算奖励（增强版本）
        支持两种模式：简单模式（原版）和增强模式
        """
        # 选择奖励计算模式
        use_enhanced_reward = self.config.get('use_enhanced_reward', False)
        
        if use_enhanced_reward and hasattr(self, 'enhanced_reward_calculator'):
            return self._calculate_enhanced_reward(action)
        else:
            return self._calculate_simple_reward()
    
    def _calculate_simple_reward(self):
        """
        计算简单奖励（原版逻辑）
        核心思想：奖励分数增量而非绝对分数，避免奖励稀疏性问题
        """
        if not self.current_obs or not self.current_obs.player_states:
            return -5.0  # 无效状态惩罚

        ps = list(self.current_obs.player_states.values())[0]
        
        # 1. 分数增量奖励（主要奖励来源）
        score_delta = ps.score - self.last_score
        score_reward = score_delta / 100.0  # 缩放系数，避免奖励过大
        
        # 2. 时间惩罚（鼓励快速决策）
        time_penalty = -0.001
        
        # 3. 死亡惩罚（关键惩罚）
        death_penalty = 0.0
        if self.engine.is_done():
            death_penalty = -10.0
        
        # 4. 生存奖励（小幅正奖励）
        survival_reward = 0.01 if not self.engine.is_done() else 0.0
        
        # 5. 尺寸奖励（可选）
        size_reward = 0.0
        if self.prev_observation and self.prev_observation.player_states:
            prev_ps = list(self.prev_observation.player_states.values())[0]
            # 基于clone列表长度的奖励（细胞数量变化）
            if hasattr(ps, 'clone') and hasattr(prev_ps, 'clone'):
                current_cells = len(ps.clone) if isinstance(ps.clone, list) else 1
                prev_cells = len(prev_ps.clone) if isinstance(prev_ps.clone, list) else 1
                cell_delta = current_cells - prev_cells
                size_reward = cell_delta * 0.1
        
        # 总奖励计算
        total_reward = (score_reward + time_penalty + death_penalty + 
                       survival_reward + size_reward)
        
        # 更新上一帧分数
        self.last_score = ps.score
        
        return total_reward
    
    def _calculate_enhanced_reward(self, action):
        """
        计算增强奖励（使用增强奖励系统）
        """
        if not hasattr(self, 'enhanced_reward_calculator'):
            # 懒加载增强奖励计算器
            try:
                from enhanced_reward_system import EnhancedRewardCalculator
                self.enhanced_reward_calculator = EnhancedRewardCalculator(self.config)
                self.reward_components_history = []
            except ImportError:
                print("⚠️  警告: 无法导入增强奖励系统，回退到简单奖励")
                return self._calculate_simple_reward()
        
        # 使用增强奖励系统计算奖励
        total_reward, reward_components = self.enhanced_reward_calculator.calculate_reward(
            self.current_obs, 
            self.prev_observation, 
            action or [0, 0, 0],  # 默认动作
            self.episode_step
        )
        
        # 记录奖励组件（用于调试）
        self.reward_components_history.append({
            'step': self.episode_step,
            'total_reward': total_reward,
            'components': reward_components
        })
        
        # 保持历史长度
        if len(self.reward_components_history) > 50:
            self.reward_components_history.pop(0)
        
        # 更新上一帧分数（保持兼容性）
        if self.current_obs and self.current_obs.player_states:
            ps = list(self.current_obs.player_states.values())[0]
            self.last_score = ps.score
        
        return total_reward
    
    def render(self, mode='human'):
        """渲染环境（可选实现）"""
        if mode == 'human':
            print(f"Frame: {self.current_obs.global_state.total_frame if self.current_obs else 0}")
            if self.current_obs and self.current_obs.player_states:
                ps = list(self.current_obs.player_states.values())[0]
                score_delta = ps.score - self.last_score
                print(f"Score: {ps.score:.2f} (Δ{score_delta:+.2f}), "
                      f"Can eject: {ps.can_eject}, Can split: {ps.can_split}")
    
    def close(self):
        """清理资源"""
        print("🔚 环境关闭")
        pass

def demo_gymnasium_usage():
    """演示标准Gymnasium使用方式（优化版）"""
    print("\n" + "="*50)
    print("🎮 GoBigger Gymnasium 环境演示 (优化版)")
    print("="*50 + "\n")
    
    # 创建环境
    env = GoBiggerEnv({'max_episode_steps': 100})
    
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
    print(f"   观察前10个值: {obs[:10]}")
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
        
        if done:
            print(f"\n🏁 回合结束 (第{step+1}步)")
            break
    
    print(f"\n✅ 演示完成！")
    print(f"   最终累计奖励: {total_reward:.4f}")
    print(f"   平均步骤奖励: {total_reward/(step+1):.4f}")
    
    env.close()

if __name__ == "__main__":
    demo_gymnasium_usage()