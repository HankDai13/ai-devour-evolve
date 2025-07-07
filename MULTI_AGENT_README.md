# 🤖 多智能体GoBigger强化学习环境

基于您在 `src_new` 中已实现的多智能体系统，这个包装器支持**RL智能体与传统AI ROBOT混合训练**，并引入**团队排名机制**作为强化学习的关键奖励信号。

## 🎯 核心特点

### 多智能体架构
- **1个RL智能体**: 通过标准Gymnasium接口进行强化学习训练
- **多个传统AI ROBOT**: 使用您已实现的稳定策略作为对手
  - `FOOD_HUNTER`: 智能食物猎手策略
  - `AGGRESSIVE`: 攻击性策略  
  - `RANDOM`: 随机移动策略

### 🏆 团队排名系统
- **实时排名**: 根据团队总分进行实时排名
- **排名奖励**: 排名提升给予正奖励，下降给予负奖励
- **竞争环境**: 多个队伍同时竞争，增加训练复杂度

### 📊 增强的观察空间 (450维)
- **全局状态** (10维): 帧数、玩家数、食物数、荆棘数等
- **RL智能体状态** (20维): 位置、速度、分数、能力状态等
- **🔥 团队排名特征** (20维): 各队分数、排名、是否为RL队伍等
- **视野内食物** (200维): 50个食物 × 4维特征
- **视野内玩家** (200维): 20个玩家 × 10维特征

## 🚀 快速开始

### 1. 编译多智能体模块

```bash
# Windows
build_multi_agent.bat

# 这将：
# 1. 配置CMake项目
# 2. 编译C++多智能体引擎
# 3. 生成Python绑定模块
# 4. 自动测试环境
```

### 2. 测试环境

```python
# 单智能体模式
python scripts/gobigger_gym_env.py

# 多智能体模式  
python scripts/gobigger_gym_env.py --multi-agent
```

### 3. 训练RL模型

```python
# 训练PPO模型对抗传统AI
python train_multi_agent.py --train

# 测试训练好的模型
python train_multi_agent.py --test
```

## 🎮 使用示例

### 基础使用

```python
from gobigger_gym_env import MultiAgentGoBiggerEnv

# 创建多智能体环境
config = {
    'ai_opponent_count': 3,  # 3个AI对手
    'max_episode_steps': 2000,
    'ai_strategies': ['FOOD_HUNTER', 'AGGRESSIVE', 'RANDOM']
}

env = MultiAgentGoBiggerEnv(config)

# 标准Gymnasium接口
obs = env.reset()
for _ in range(1000):
    action = env.action_space.sample()  # [move_x, move_y, action_type]
    obs, reward, done, info = env.step(action)
    
    if done:
        print(f"最终排名: {info['final_rank']}/{info['total_teams']}")
        break

env.close()
```

### 与stable-baselines3训练

```python
from stable_baselines3 import PPO
from stable_baselines3.common.env_util import make_vec_env

# 创建向量化环境
def make_env():
    return MultiAgentGoBiggerEnv({
        'ai_opponent_count': 3,
        'max_episode_steps': 2000
    })

env = make_vec_env(make_env, n_envs=4)

# 训练PPO模型
model = PPO("MlpPolicy", env, verbose=1)
model.learn(total_timesteps=100000)
model.save("multi_agent_ppo")
```

## 🏆 奖励机制

多智能体环境使用**复合奖励函数**，核心创新是**团队排名奖励**：

```python
def calculate_reward():
    # 1. 基础分数奖励
    score_reward = score_delta / 100.0
    
    # 2. 🔥 团队排名奖励（核心创新）
    rank_reward = (total_teams - current_rank + 1) / total_teams * 1.0
    
    # 3. 排名变化奖励
    if current_rank < last_rank:
        rank_change_reward = 3.0    # 排名提升
    elif current_rank > last_rank:
        rank_change_reward = -2.0   # 排名下降
    
    # 4. 生存/死亡奖励
    survival_reward = 0.01 if alive else -10.0
    
    # 5. AI交互奖励
    if defeated_ai_opponent:
        ai_interaction_reward = 5.0
    
    return sum(all_rewards)
```

## 🤖 AI对手策略

### FOOD_HUNTER (食物猎手)
- 智能寻找食物聚集区域
- 评估食物密度和安全性
- 在威胁较小时积极收集食物

### AGGRESSIVE (攻击型)
- 主动寻找可以吞噬的目标
- 威胁评估和逃跑机制
- 使用分裂进行攻击和逃跑

### RANDOM (随机型)
- 随机移动和动作
- 提供不可预测的对手
- 作为基准策略

## 📊 环境配置

```python
config = {
    # 基础游戏配置
    'max_episode_steps': 2000,        # 最大步数
    'max_frames': 4000,               # 游戏最大帧数
    'update_interval': 16,            # 更新间隔(ms)
    
    # 多智能体配置
    'ai_opponent_count': 3,           # AI对手数量
    'ai_strategies': [                # AI策略列表
        'FOOD_HUNTER', 
        'AGGRESSIVE', 
        'RANDOM'
    ],
    
    # 游戏世界配置
    'max_food_count': 3000,           # 最大食物数
    'init_food_count': 2500,          # 初始食物数
    'max_thorns_count': 12,           # 最大荆棘数
    'init_thorns_count': 9,           # 初始荆棘数
    
    # 调试配置
    'debug_mode': False,              # 调试模式
}
```

## 🔧 技术架构

### C++后端 (src_new/)
- `GameManager`: 核心游戏逻辑管理
- `MultiPlayerManager`: 多玩家游戏管理
- `SimpleAIPlayer`: 传统AI策略实现
- `CloneBall`: 玩家球体物理和逻辑

### Python绑定层
- `multi_agent_bindings.cpp`: pybind11 C++绑定
- `multi_agent_game_engine.cpp`: 多智能体引擎包装器

### Python环境层
- `MultiAgentGoBiggerEnv`: Gymnasium兼容的环境类
- 标准的 `reset()`, `step()`, `render()` 接口
- 增强的观察空间和奖励函数

## 📈 性能优化

### 并行训练
- 支持多个并行环境 (`make_vec_env`)
- 每个环境独立运行多智能体对战
- 显著加速训练过程

### 内存优化
- 固定大小的观察向量 (450维)
- 高效的特征提取和归一化
- 智能的视野范围限制

### 计算优化
- C++核心引擎提供高性能
- 优化的碰撞检测和物理计算
- 智能的AI决策间隔控制

## 🎯 训练建议

### 超参数
```python
# PPO推荐配置
PPO_CONFIG = {
    'learning_rate': 3e-4,
    'n_steps': 2048,
    'batch_size': 256,
    'n_epochs': 10,
    'gamma': 0.99,
    'gae_lambda': 0.95,
    'clip_range': 0.2,
    'ent_coef': 0.01,      # 鼓励探索
    'vf_coef': 0.5,
}
```

### 网络架构
```python
# 推荐的神经网络架构
policy_kwargs = dict(
    net_arch=dict(
        pi=[512, 512, 256],    # Actor网络
        vf=[512, 512, 256]     # Critic网络
    ),
    activation_fn=torch.nn.ReLU,
)
```

### 训练策略
1. **渐进式训练**: 从简单AI开始，逐步增加难度
2. **课程学习**: 先短episode，再长episode
3. **对手多样性**: 使用不同AI策略组合
4. **奖励平衡**: 调整排名奖励权重

## 🔍 调试和监控

### 调试模式
```python
env = MultiAgentGoBiggerEnv({'debug_mode': True})
# 启用详细的训练日志和状态输出
```

### 渲染和可视化
```python
env.render()  # 显示当前游戏状态
# 输出：
# 🎮 Frame: 1234
# 🎯 RL Agent - Score: 5432.1, Pos: (123, 456)
# 🏆 Team Ranking:
#   1. 👑 RL Team 0: 5432.1
#   2. 🤖 AI Team 1: 4321.0
#   3. 🤖 AI Team 2: 3210.5
```

### 训练监控
- TensorBoard日志支持
- 定期模型检查点保存
- 评估回调和性能指标

## 📚 扩展和定制

### 添加新的AI策略
1. 在 `SimpleAIPlayer` 中实现新策略
2. 更新Python绑定
3. 在环境配置中添加策略名称

### 修改奖励函数
1. 继承 `MultiAgentGoBiggerEnv`
2. 重写 `_calculate_multi_agent_reward()` 方法
3. 添加自定义奖励组件

### 扩展观察空间
1. 修改 `_extract_multi_agent_features()` 方法
2. 调整 `observation_space_shape`
3. 更新特征提取逻辑

## 🚨 常见问题

### Q: 编译失败怎么办？
A: 检查Qt路径和CMake配置，确保所有依赖都已安装。

### Q: 多智能体模块导入失败？
A: 运行 `build_multi_agent.bat` 重新编译，检查Python路径设置。

### Q: 训练收敛慢？
A: 调整奖励函数权重，增加熵系数鼓励探索，使用更多并行环境。

### Q: AI对手太强/太弱？
A: 调整AI策略组合，修改AI决策间隔，或实现自定义AI策略。

## 🔮 未来扩展

- [ ] 支持更多AI策略类型
- [ ] 实现动态难度调整
- [ ] 添加多队伍合作模式
- [ ] 集成更高级的强化学习算法
- [ ] 支持自对弈训练
- [ ] 实现分层强化学习

---

🎉 **享受多智能体强化学习的乐趣！** 这个环境结合了您已实现的稳定AI策略和现代强化学习技术，为训练强大的AI智能体提供了理想的平台。
