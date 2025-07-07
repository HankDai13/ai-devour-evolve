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

def setup_qt_environment():
    """Setup Qt environment variables for proper plugin loading"""
    qt_dir = r"D:\qt\6.9.1\msvc2022_64"
    
    # Set Qt environment variables
    os.environ['QT_QPA_PLATFORM_PLUGIN_PATH'] = os.path.join(qt_dir, "plugins", "platforms")
    os.environ['QT_PLUGIN_PATH'] = os.path.join(qt_dir, "plugins")
    
    # Add Qt bin to PATH
    qt_bin = os.path.join(qt_dir, "bin")
    if qt_bin not in os.environ.get('PATH', ''):
        os.environ['PATH'] = qt_bin + os.pathsep + os.environ.get('PATH', '')

# Setup Qt environment before any imports that might use Qt
setup_qt_environment()

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

# 添加python目录到搜索路径（用于多智能体模块）
python_dir = root_dir / "python"
if python_dir.exists():
    sys.path.insert(0, str(python_dir))
    print(f"✅ 添加Python模块目录: {python_dir}")

if not build_dir_found:
    raise ImportError("❌ 未找到有效的构建目录。请确保项目已编译。")

try:
    import gobigger_env
    print("✅ 成功导入 gobigger_env 模块")
except ImportError as e:
    print(f"❌ 导入 gobigger_env 失败: {e}")
    print(f"搜索路径: {build_dir_found}")
    raise

# 尝试导入多智能体模块（可选）
try:
    import gobigger_multi_env
    MULTI_AGENT_AVAILABLE = True
    print("✅ 成功导入 gobigger_multi_env 模块（多智能体支持）")
except ImportError:
    MULTI_AGENT_AVAILABLE = False
    print("⚠️  gobigger_multi_env 不可用，仅支持单智能体模式")

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
            return self._calculate_simple_reward(action)
    
    def _calculate_simple_reward(self, action=None):
        """
        计算简单奖励（原版逻辑 + Gemini优化的事件驱动奖励）
        核心思想：奖励分数增量而非绝对分数，避免奖励稀疏性问题
        🔥 新增：大幅奖励Split和Eject高级动作，解决策略崩塌问题
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
        
        # 🔥 5. 事件驱动的高级动作奖励（Gemini推荐）
        advanced_action_reward = 0.0
        if action is not None and len(action) >= 3:
            action_type = int(np.round(np.clip(action[2], 0, 2)))
            
            # Split动作奖励（动作类型1）
            if action_type == 1:
                base_split_reward = 2.0  # 基础Split奖励
                # 如果Split后分数确实增加，给予额外奖励
                if score_delta > 0:
                    advanced_action_reward += base_split_reward + score_delta / 50.0
                    if hasattr(self, 'debug_rewards'):
                        print(f"🎯 Split成功! 奖励: {base_split_reward + score_delta / 50.0:.3f}")
                else:
                    # 即使Split没有立即获得分数，也给予小奖励鼓励探索
                    advanced_action_reward += base_split_reward * 0.3
                    if hasattr(self, 'debug_rewards'):
                        print(f"🔄 Split尝试! 奖励: {base_split_reward * 0.3:.3f}")
            
            # Eject动作奖励（动作类型2）
            elif action_type == 2:
                base_eject_reward = 1.5  # 基础Eject奖励
                # 如果Eject后分数增加（可能喂食队友或战略使用），给予奖励
                if score_delta >= 0:  # 不惩罚Eject，鼓励战略使用
                    advanced_action_reward += base_eject_reward
                    if hasattr(self, 'debug_rewards'):
                        print(f"💨 Eject使用! 奖励: {base_eject_reward:.3f}")
                else:
                    # 即使损失分数，也给小奖励避免过度惩罚
                    advanced_action_reward += base_eject_reward * 0.2
        
        # 6. 尺寸奖励（基于细胞数量变化）
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
                       survival_reward + size_reward + advanced_action_reward)
        
        # 更新上一帧分数
        self.last_score = ps.score
        
        return total_reward
        
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

class MultiAgentGoBiggerEnv(gym.Env if GYMNASIUM_AVAILABLE else gym.Env):
    """
    多智能体GoBigger环境（RL Agent vs 传统AI ROBOT）
    
    这个环境整合了你在src_new中实现的多智能体系统，支持：
    1. 一个RL智能体通过Gymnasium接口训练
    2. 多个传统AI策略机器人作为稳定对手
    3. 团队排名机制作为奖励信号
    4. 与单智能体环境兼容的接口
    """
    
    def __init__(self, config=None):
        """初始化多智能体环境"""
        super().__init__()
        
        if not MULTI_AGENT_AVAILABLE:
            raise ImportError("多智能体功能不可用，请确保 gobigger_multi_env 模块已编译")
        
        self.config = config or {}
        
        # 创建多智能体引擎配置
        engine_config = gobigger_multi_env.MultiAgentConfig()
        engine_config.maxFoodCount = self.config.get('max_food_count', 3000)
        engine_config.initFoodCount = self.config.get('init_food_count', 2500)
        engine_config.maxThornsCount = self.config.get('max_thorns_count', 12)
        engine_config.initThornsCount = self.config.get('init_thorns_count', 9)
        engine_config.maxFrames = self.config.get('max_frames', 3000)
        engine_config.aiOpponentCount = self.config.get('ai_opponent_count', 3)
        engine_config.gameUpdateInterval = self.config.get('update_interval', 16)
        
        # 创建C++多智能体引擎
        self.engine = gobigger_multi_env.MultiAgentGameEngine(engine_config)
        
        # 定义动作和观察空间（与单智能体保持兼容）
        self.action_space = spaces.Box(
            low=np.array([-1.0, -1.0, 0], dtype=np.float32),
            high=np.array([1.0, 1.0, 2], dtype=np.float32),
            dtype=np.float32
        )
        
        # 增强的观察空间，包含团队排名信息
        self.observation_space_shape = (450,)
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
        
        # 传统AI策略信息
        self.ai_strategies = self.config.get('ai_strategies', [
            'FOOD_HUNTER', 'AGGRESSIVE', 'RANDOM'
        ])
        
        print(f"🤖 多智能体环境初始化完成")
        print(f"   RL智能体 vs {engine_config.aiOpponentCount} 个传统AI")
        print(f"   AI策略: {self.ai_strategies}")
        
    def reset(self, seed=None, options=None):
        """重置环境"""
        if seed is not None:
            np.random.seed(seed)
        
        # 重置多智能体引擎
        self.current_obs = self.engine.reset()
        self.episode_step = 0
        self.episode_reward = 0.0
        
        # 初始化奖励追踪
        reward_info = self.engine.get_reward_info()
        self.last_score = reward_info.get('score', 0.0)
        self.initial_score = self.last_score
        self.last_team_rank = reward_info.get('team_rank', 1)
        
        # 兼容不同版本的gym
        if GYMNASIUM_AVAILABLE:
            return self._extract_multi_agent_features(), {}
        else:
            return self._extract_multi_agent_features()
    
    def step(self, action):
        """执行一步动作"""
        # 格式化RL智能体动作
        if isinstance(action, (list, tuple, np.ndarray)) and len(action) >= 3:
            move_x = float(np.clip(action[0], -1.0, 1.0))
            move_y = float(np.clip(action[1], -1.0, 1.0))
            action_type = int(np.round(np.clip(action[2], 0, 2)))
            rl_action = [move_x, move_y, action_type]
        else:
            rl_action = [0.0, 0.0, 0]
        
        # 构造多智能体动作字典
        actions_dict = {"rl_agent": rl_action}
        
        # 执行动作（传统AI会自动执行）
        self.current_obs = self.engine.step(actions_dict)
        self.episode_step += 1
        
        # 计算多智能体奖励
        reward = self._calculate_multi_agent_reward()
        
        # 检查终止条件
        terminated = self.engine.is_done()
        truncated = self.episode_step >= self.max_episode_steps
        
        # 累积奖励
        self.episode_reward += reward
        
        # 准备info字典
        info = {}
        if terminated or truncated:
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
        
        # 兼容不同版本的gym
        if GYMNASIUM_AVAILABLE:
            return self._extract_multi_agent_features(), reward, terminated, truncated, info
        else:
            return self._extract_multi_agent_features(), reward, terminated or truncated, info
    
    def _extract_multi_agent_features(self):
        """提取多智能体特征向量"""
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
        features.extend([0.0] * 6)  # 预留特征
        
        # 2. RL智能体状态特征 (20维)
        rl_obs = self.current_obs.get('rl_agent', {})
        if rl_obs:
            pos = rl_obs.get('position', (0, 0))
            vel = rl_obs.get('velocity', (0, 0))
            features.extend([
                pos[0] / 2000.0, pos[1] / 2000.0,
                rl_obs.get('radius', 0) / 100.0,
                rl_obs.get('score', 0) / 10000.0,
                vel[0] / 1000.0, vel[1] / 1000.0,
                float(rl_obs.get('can_split', False)),
                float(rl_obs.get('can_eject', False)),
            ])
            features.extend([0.0] * 12)  # 预留
        else:
            features.extend([0.0] * 20)  # 死亡时填0
        
        # 3. 🔥 团队排名特征 (20维) - 核心多智能体特征
        team_ranking = global_state.get('team_ranking', [])
        ranking_features = [0.0] * 20
        
        for i, team_info in enumerate(team_ranking[:5]):
            base_idx = i * 4
            if base_idx + 3 < len(ranking_features):
                ranking_features[base_idx] = team_info.get('team_id', 0) / 10.0
                ranking_features[base_idx + 1] = team_info.get('score', 0) / 10000.0
                ranking_features[base_idx + 2] = team_info.get('rank', 1) / 5.0
                ranking_features[base_idx + 3] = 1.0 if team_info.get('team_id', -1) == 0 else 0.0
        
        features.extend(ranking_features)
        
        # 4. 视野内食物特征 (200维: 50个食物 × 4维)
        nearby_food = rl_obs.get('nearby_food', [])
        food_features = []
        for i in range(50):
            if i < len(nearby_food):
                food = nearby_food[i]
                food_features.extend([
                    food[0] / 2000.0, food[1] / 2000.0,
                    food[2] / 10.0, food[3] / 100.0
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
                    player[0] / 2000.0, player[1] / 2000.0,
                    player[2] / 100.0, player[3] / 10000.0,
                    player[4] / 10.0, player[5] / 10.0
                ])
                player_features.extend([0.0] * 4)  # 预留
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
    
    def _calculate_multi_agent_reward(self):
        """
        计算多智能体奖励（包含团队排名奖励）
        🔥 这是多智能体环境的核心创新：将团队排名作为奖励信号
        """
        reward_info = self.engine.get_reward_info()
        
        # 1. 基础分数奖励
        current_score = reward_info.get('score', 0.0)
        score_delta = current_score - self.last_score
        score_reward = score_delta / 100.0
        
        # 2. 🔥 团队排名奖励（核心多智能体特征）
        current_rank = reward_info.get('team_rank', 1)
        total_teams = reward_info.get('total_teams', 1)
        
        # 安全检查：避免除零错误
        if total_teams <= 0:
            total_teams = 1
            current_rank = 1
        
        # 排名奖励：排名越高奖励越大
        rank_reward = (total_teams - current_rank + 1) / total_teams * 1.0
        
        # 排名变化奖励：排名提升给予额外奖励
        rank_change_reward = 0.0
        if current_rank < self.last_team_rank:
            rank_change_reward = 3.0  # 排名提升奖励
        elif current_rank > self.last_team_rank:
            rank_change_reward = -2.0  # 排名下降惩罚
        
        # 3. 生存奖励
        is_alive = reward_info.get('alive', False)
        survival_reward = 0.01 if is_alive else -10.0
        
        # 4. 时间惩罚
        time_penalty = -0.001
        
        # 5. 🔥 传统AI交互奖励（鼓励与AI对手的有效交互）
        ai_interaction_reward = 0.0
        ai_states = self.current_obs.get('ai_states', {})
        alive_ai_count = len([s for s in ai_states.values() 
                             if s.get('alive_balls_count', 0) > 0])
        
        # 如果击败了AI对手，给予奖励
        if hasattr(self, '_last_alive_ai_count'):
            if alive_ai_count < self._last_alive_ai_count:
                ai_interaction_reward = 5.0  # 击败AI对手奖励
        self._last_alive_ai_count = alive_ai_count
        
        # 总奖励
        total_reward = (score_reward + rank_reward + rank_change_reward + 
                       survival_reward + time_penalty + ai_interaction_reward)
        
        # 更新状态
        self.last_score = current_score
        self.last_team_rank = current_rank
        
        return total_reward
    
    def render(self, mode='human'):
        """渲染环境（增强版本，显示多智能体信息）"""
        if mode == 'human' and self.current_obs:
            global_state = self.current_obs.get('global_state', {})
            rl_obs = self.current_obs.get('rl_agent', {})
            
            print(f"🎮 Frame: {global_state.get('frame', 0)}")
            
            # RL智能体状态
            if rl_obs:
                pos = rl_obs.get('position', (0, 0))
                print(f"🎯 RL Agent - Score: {rl_obs.get('score', 0):.1f}, "
                      f"Pos: ({pos[0]:.0f}, {pos[1]:.0f})")
            
            # 团队排名
            team_ranking = global_state.get('team_ranking', [])
            print("🏆 Team Ranking:")
            for i, team in enumerate(team_ranking):
                marker = "👑 RL" if team.get('team_id') == 0 else "🤖 AI"
                print(f"  {i+1}. {marker} Team {team.get('team_id', 0)}: {team.get('score', 0):.1f}")
            
            # AI对手状态
            ai_states = self.current_obs.get('ai_states', {})
            alive_ai = len([s for s in ai_states.values() 
                           if s.get('alive_balls_count', 0) > 0])
            print(f"🤖 存活AI对手: {alive_ai}/{len(ai_states)}")
    
    def close(self):
        """清理资源"""
        print("🔚 多智能体环境关闭")

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

def demo_multi_agent_usage():
    """演示多智能体环境（RL vs 传统AI ROBOT）"""
    print("\n" + "="*70)
    print("🤖 多智能体 GoBigger 环境演示 (RL Agent vs 传统AI ROBOT)")
    print("="*70 + "\n")
    
    if not MULTI_AGENT_AVAILABLE:
        print("❌ 多智能体功能不可用，请确保编译了多智能体模块")
        return
    
    # 创建多智能体环境
    config = {
        'max_episode_steps': 150,
        'ai_opponent_count': 3,        # 3个传统AI对手
        'max_frames': 800,
        'ai_strategies': ['FOOD_HUNTER', 'AGGRESSIVE', 'RANDOM']
    }
    
    env = MultiAgentGoBiggerEnv(config)
    
    # 检查环境
    try:
        from stable_baselines3.common.env_checker import check_env
        print("🔍 使用 stable-baselines3 检查多智能体环境...")
        check_env(env)
        print("✅ 多智能体环境检查通过！\n")
    except ImportError:
        print("⚠️ 未安装 stable-baselines3，跳过环境检查\n")
    
    # 重置环境
    if GYMNASIUM_AVAILABLE:
        obs, info = env.reset()
    else:
        obs = env.reset()
        info = {}
    
    print(f"🏁 多智能体环境重置完成")
    print(f"   观察空间: {env.observation_space}")
    print(f"   动作空间: {env.action_space}")
    print(f"   观察维度: {obs.shape}")
    print(f"   AI对手策略: {config['ai_strategies']}")
    print()
    
    # 运行几步，展示RL vs 传统AI的交互
    total_reward = 0
    best_rank = float('inf')
    
    for step in range(15):
        # 智能动作策略：前几步探索，后面更激进
        if step < 5:
            # 探索阶段：随机移动 + 偶尔分裂
            action = env.action_space.sample()
            action[2] = 1 if step == 3 and np.random.random() > 0.5 else 0  # 偶尔分裂
        else:
            # 激进阶段：更多高级动作
            angle = step * 0.5  # 螺旋移动
            action = np.array([
                np.cos(angle) * 0.8,  # x方向
                np.sin(angle) * 0.8,  # y方向
                1 if step % 7 == 0 else (2 if step % 5 == 0 else 0)  # 分裂或喷射
            ], dtype=np.float32)
        
        if GYMNASIUM_AVAILABLE:
            obs, reward, terminated, truncated, info = env.step(action)
            done = terminated or truncated
        else:
            obs, reward, done, info = env.step(action)
            terminated, truncated = done, False
        
        total_reward += reward
        
        # 获取当前排名
        current_rank = info.get('team_rank', 1) if 'team_rank' in env.engine.get_reward_info() else 1
        if current_rank < best_rank:
            best_rank = current_rank
        
        print(f"步骤 {step+1:02d}: "
              f"动作=[{action[0]:.2f}, {action[1]:.2f}, {['移动','分裂','喷射'][int(np.round(action[2]))]}], "
              f"奖励={reward:.4f}, "
              f"累计奖励={total_reward:.4f}")
        
        # 每5步渲染一次，显示多智能体状态
        if step % 5 == 0:
            print("\n" + "-" * 50)
            env.render()
            print("-" * 50 + "\n")
        
        if done:
            print(f"\n🏁 多智能体回合结束 (第{step+1}步)")
            if 'final_rank' in info:
                print(f"   🏆 最终排名: {info['final_rank']}/{info.get('total_teams', 1)}")
                print(f"   📊 分数变化: {info.get('score_delta', 0):+.1f}")
                print(f"   🤖 存活AI对手: {info.get('ai_opponents_alive', 0)}")
                print(f"   📈 排名进度: {info.get('rank_progress', 0)*100:.1f}%")
            break
    
    print(f"\n✅ 多智能体演示完成！")
    print(f"   🎯 最终累计奖励: {total_reward:.4f}")
    print(f"   📊 平均步骤奖励: {total_reward/(step+1):.4f}")
    print(f"   🏆 最佳排名: {best_rank}")
    print(f"   🤖 AI对手类型: FOOD_HUNTER(食物猎手), AGGRESSIVE(攻击型), RANDOM(随机)")
    
    env.close()

if __name__ == "__main__":
    import sys
    
    # 检查命令行参数
    if len(sys.argv) > 1 and sys.argv[1] == "--multi-agent":
        print("🚀 启动多智能体模式演示...")
        demo_multi_agent_usage()
    else:
        print("🚀 启动单智能体模式演示...")
        print("💡 提示: 使用 --multi-agent 参数启动多智能体模式")
        demo_gymnasium_usage()
    demo_multi_agent_usage()