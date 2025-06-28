# GoBigger 强化学习训练指南

## 📋 目录

1. [快速开始](#快速开始)
2. [环境配置](#环境配置)
3. [训练脚本使用](#训练脚本使用)
4. [参数配置详解](#参数配置详解)
5. [训练优化指南](#训练优化指南)
6. [评估和测试](#评估和测试)
7. [故障排除](#故障排除)
8. [进阶优化](#进阶优化)

---

## 🚀 快速开始

### 1. 环境准备

首先确保您已经完成了基础环境搭建：

```bash
# 1. 构建C++核心库
# 在VS Code中运行任务: "Build Main Application"
# 或使用命令行:
cd d:\Coding\Projects\ai-devour-evolve
D:\qt\Tools\CMake_64\bin\cmake.exe --build build --config Release

# 2. 安装Python依赖
pip install stable-baselines3
pip install gymnasium
pip install numpy
pip install rich  # 用于美化界面
```

### 2. 快速训练

```bash
# 进入脚本目录
cd scripts

# 运行简化界面训练（推荐初学者）
python train_simple_ui.py

# 或运行Rich美化界面训练
python train_rich_ui.py

# 或运行基础训练脚本
python train_rl_agent.py
```

### 3. 验证训练效果

```bash
# 测试训练好的智能体
python test_agent_score.py
```

---

## 🔧 环境配置

### Python环境设置

项目使用以下核心库：

- **stable-baselines3**: PPO强化学习算法
- **gymnasium**: 标准化RL环境接口
- **numpy**: 数值计算
- **rich**: 终端美化界面（可选）

### C++构建确认

确保以下文件存在：
```
build/Release/
├── gobigger_env.pyd          # Python绑定库
├── ai-devour-evolve.exe      # 主程序
└── *.dll                     # 依赖库
```

---

## 🎮 训练脚本使用

### 1. train_simple_ui.py（推荐初学者）

**特点**: 兼容性强，ASCII美化界面，实时进度显示

```bash
python train_simple_ui.py
```

**界面示例**:
```
🎮 GoBigger 强化学习训练
═══════════════════════════════
训练进度: [████████████████████] 100% (20000/20000)
Episodes: 15 | 最佳分数: 1250 | 平均分数: 890
FPS: 1200 | 时长: 18s
```

### 2. train_rich_ui.py（推荐有Rich环境）

**特点**: 精美表格界面，实时统计，多线程更新

```bash
python train_rich_ui.py
```

**界面特色**:
- 🎯 实时分数统计表格
- 📊 训练指标监控
- 📈 可视化进度条
- 🏆 最佳成绩追踪

### 3. train_rl_agent.py（基础版本）

**特点**: 命令行输出，简单直接

```bash
python train_rl_agent.py
```

---

## ⚙️ 参数配置详解

### 1. 环境参数配置

在 `scripts/gobigger_gym_env.py` 中：

```python
# 环境基础配置
default_config = {
    'max_episode_steps': 1500,     # 每局最大步数
}

# 奖励函数参数
class GoBiggerEnv:
    def __init__(self, config=None):
        # 奖励权重
        self.score_reward_scale = 0.01      # 分数奖励缩放
        self.food_reward = 0.5              # 吃食物奖励
        self.survival_reward = 0.001        # 存活奖励
        self.death_penalty = -5.0           # 死亡惩罚
        self.border_penalty = -0.1          # 撞边界惩罚
```

#### 关键参数说明：

| 参数 | 默认值 | 说明 | 调整建议 |
|------|--------|------|----------|
| `max_episode_steps` | 1500 | 每局游戏最大步数 | 增加训练更久，减少训练更快 |
| `score_reward_scale` | 0.01 | 分数变化的奖励缩放 | 提高让智能体更关注分数 |
| `food_reward` | 0.5 | 成功吃到食物的额外奖励 | 增加鼓励觅食行为 |
| `survival_reward` | 0.001 | 每步存活的基础奖励 | 避免智能体过早死亡 |
| `death_penalty` | -5.0 | 死亡时的惩罚 | 增加绝对值让智能体更谨慎 |

### 2. PPO算法参数配置

在训练脚本中：

```python
model = PPO(
    "MlpPolicy",                    # 策略网络类型
    env,
    learning_rate=3e-4,             # 学习率
    n_steps=1024,                   # 每次更新收集的步数
    batch_size=64,                  # 批处理大小
    n_epochs=10,                    # 每次更新的训练轮数
    gamma=0.99,                     # 折扣因子
    gae_lambda=0.95,                # GAE参数
    clip_range=0.2,                 # PPO裁剪范围
    ent_coef=0.01,                  # 熵正则化系数
    vf_coef=0.5,                    # 价值函数损失系数
    verbose=1
)
```

#### PPO参数调整指南：

| 参数 | 默认值 | 作用 | 调整策略 |
|------|--------|------|----------|
| `learning_rate` | 3e-4 | 学习速度 | 过慢增加，不稳定减少 |
| `n_steps` | 1024 | 经验收集量 | 增加提升稳定性，减少加快训练 |
| `batch_size` | 64 | 批处理大小 | 根据内存和稳定性调整 |
| `n_epochs` | 10 | 每批数据训练轮数 | 增加提升学习，但可能过拟合 |
| `gamma` | 0.99 | 未来奖励折扣 | 接近1重视长期，远离1重视短期 |
| `clip_range` | 0.2 | 策略更新限制 | 减少更保守，增加更激进 |

### 3. 训练总步数配置

```python
# 在训练脚本中修改
total_timesteps = 20000    # 总训练步数
```

**建议设置**:
- **快速测试**: 5,000 - 10,000 步
- **正常训练**: 20,000 - 50,000 步  
- **深度训练**: 100,000+ 步

---

## 📈 训练优化指南

### 1. 初学者优化路径

#### 第一阶段：基础训练
```python
# 保守配置，确保稳定训练
model = PPO(
    "MlpPolicy", env,
    learning_rate=1e-4,        # 较小学习率
    n_steps=512,               # 较少步数
    batch_size=32,             # 较小批次
    n_epochs=5,                # 较少轮数
    verbose=1
)
```

#### 第二阶段：参数调优
观察训练效果后调整：
- 如果训练太慢 → 增加 `learning_rate` 到 3e-4
- 如果不稳定 → 减少 `learning_rate` 到 1e-5
- 如果内存充足 → 增加 `n_steps` 到 1024

#### 第三阶段：高级优化
```python
# 优化后配置
model = PPO(
    "MlpPolicy", env,
    learning_rate=3e-4,
    n_steps=2048,              # 更多经验
    batch_size=128,            # 更大批次
    n_epochs=15,               # 更多训练
    gamma=0.995,               # 更重视长期奖励
    gae_lambda=0.98,
    ent_coef=0.005,            # 减少探索
    verbose=1
)
```

### 2. 环境参数优化

#### 奖励函数调优
```python
# 在 gobigger_gym_env.py 中修改
class GoBiggerEnv:
    def __init__(self, config=None):
        # 阶段1：基础奖励
        self.score_reward_scale = 0.01
        self.food_reward = 0.5
        
        # 阶段2：如果智能体不够积极觅食
        self.score_reward_scale = 0.02     # 增加分数奖励
        self.food_reward = 1.0             # 增加食物奖励
        
        # 阶段3：如果智能体过于冒险
        self.death_penalty = -10.0         # 增加死亡惩罚
        self.border_penalty = -0.5         # 增加边界惩罚
```

#### Episode长度调优
```python
# 短训练：快速验证算法
config = {'max_episode_steps': 500}

# 标准训练：平衡效果和时间
config = {'max_episode_steps': 1500}

# 长训练：深度策略学习
config = {'max_episode_steps': 3000}
```

### 3. 监控训练效果

#### 关键指标观察
1. **Episode分数趋势**：应该逐渐上升
2. **Episode长度**：应该趋于稳定或增长
3. **奖励均值**：应该持续改善
4. **策略熵**：初期高，后期逐渐降低

#### 训练效果判断
```bash
# 好的训练迹象：
✅ 平均分数持续上升
✅ 智能体能稳定吃到食物 
✅ Episode长度逐渐增加
✅ 死亡频率逐渐降低

# 需要调整的迹象：
❌ 分数长期不变或下降
❌ 智能体频繁死亡
❌ 训练loss不收敛
❌ 智能体行为异常（如只移动不觅食）
```

---

## 🧪 评估和测试

### 1. 快速评估脚本

```bash
# 测试智能体分数表现
python test_agent_score.py

# 观察智能体游戏过程
python debug_observation.py
```

### 2. 自定义评估

```python
# 创建自定义评估脚本
from stable_baselines3 import PPO
from gobigger_gym_env import GoBiggerEnv

# 加载训练好的模型
model = PPO.load("models/PPO_gobigger.zip")
env = GoBiggerEnv()

# 运行评估
episodes = 10
total_scores = []

for i in range(episodes):
    obs, _ = env.reset()
    episode_score = 0
    done = False
    
    while not done:
        action, _ = model.predict(obs, deterministic=True)
        obs, reward, terminated, truncated, info = env.step(action)
        done = terminated or truncated
        
        if 'final_score' in info:
            episode_score = info['final_score']
    
    total_scores.append(episode_score)
    print(f"Episode {i+1}: Score = {episode_score:.0f}")

print(f"\n平均分数: {np.mean(total_scores):.0f}")
print(f"最佳分数: {np.max(total_scores):.0f}")
```

### 3. 性能基准

#### 智能体水平判断：
- **初学者级别**: 平均分数 < 500
- **中等水平**: 平均分数 500-1500
- **高级水平**: 平均分数 1500-3000
- **专家级别**: 平均分数 > 3000

---

## 🔧 故障排除

### 常见问题和解决方案

#### 1. 导入错误
```bash
# 问题：ModuleNotFoundError: No module named 'gobigger_env'
# 解决：确保C++库已正确构建
cd d:\Coding\Projects\ai-devour-evolve
D:\qt\Tools\CMake_64\bin\cmake.exe --build build --config Release
```

#### 2. Rich库安装问题
```bash
# 问题：导入rich失败
# 解决：
pip install rich

# 或使用conda
conda install rich -c conda-forge
```

#### 3. 训练过慢或不稳定
```python
# 问题：训练速度太慢
# 解决：减少n_steps和batch_size
model = PPO("MlpPolicy", env, n_steps=256, batch_size=32)

# 问题：训练不稳定
# 解决：降低学习率
model = PPO("MlpPolicy", env, learning_rate=1e-5)
```

#### 4. 智能体不学习
```python
# 检查奖励函数
# 在gobigger_gym_env.py中添加调试输出
def step(self, action):
    # ...existing code...
    print(f"Debug: reward={reward}, score_change={score_change}")
    return obs, reward, terminated, truncated, info
```

#### 5. 内存不足
```python
# 减少并行环境数量
env = make_vec_env(lambda: create_env(), n_envs=1)  # 从4改为1

# 减少经验缓冲区大小
model = PPO("MlpPolicy", env, n_steps=512, batch_size=32)
```

---

## 🚀 进阶优化

### 1. 网络架构优化

```python
# 自定义策略网络
from stable_baselines3.ppo import MlpPolicy
from stable_baselines3.common.torch_layers import BaseFeaturesExtractor
import torch.nn as nn

class CustomFeatureExtractor(BaseFeaturesExtractor):
    def __init__(self, observation_space, features_dim=256):
        super().__init__(observation_space, features_dim)
        self.linear = nn.Sequential(
            nn.Linear(observation_space.shape[0], 512),
            nn.ReLU(),
            nn.Linear(512, 256),
            nn.ReLU(),
            nn.Linear(256, features_dim),
            nn.ReLU()
        )
    
    def forward(self, observations):
        return self.linear(observations)

# 使用自定义网络
policy_kwargs = dict(
    features_extractor_class=CustomFeatureExtractor,
    features_extractor_kwargs=dict(features_dim=256),
    net_arch=[dict(pi=[256, 128], vf=[256, 128])]
)

model = PPO("MlpPolicy", env, policy_kwargs=policy_kwargs)
```

### 2. 课程学习

```python
# 渐进式难度训练
class CurriculumEnv(GoBiggerEnv):
    def __init__(self, stage=1):
        super().__init__()
        self.stage = stage
        
    def reset(self, **kwargs):
        obs, info = super().reset(**kwargs)
        
        # 根据阶段调整难度
        if self.stage == 1:
            # 简单模式：更多食物，更小地图
            pass
        elif self.stage == 2:
            # 中等模式：标准配置
            pass
        elif self.stage == 3:
            # 困难模式：更少食物，更大地图
            pass
            
        return obs, info

# 分阶段训练
for stage in [1, 2, 3]:
    print(f"Training Stage {stage}")
    env = CurriculumEnv(stage=stage)
    model.set_env(env)
    model.learn(total_timesteps=10000)
```

### 3. 多智能体训练

```python
# 同时训练多个智能体
def create_multi_agent_env():
    """创建多智能体环境"""
    # 可以扩展为多玩家对战环境
    pass

# 使用自博弈提升性能
def self_play_training():
    """自博弈训练"""
    # 让智能体与自己的历史版本对战
    pass
```

### 4. 超参数优化

```python
# 使用Optuna进行超参数搜索
import optuna

def objective(trial):
    # 定义搜索空间
    learning_rate = trial.suggest_float("learning_rate", 1e-5, 1e-3, log=True)
    n_steps = trial.suggest_categorical("n_steps", [512, 1024, 2048])
    batch_size = trial.suggest_categorical("batch_size", [32, 64, 128])
    
    # 训练模型
    model = PPO("MlpPolicy", env, 
                learning_rate=learning_rate,
                n_steps=n_steps, 
                batch_size=batch_size)
    
    model.learn(total_timesteps=10000)
    
    # 评估性能
    mean_reward = evaluate_model(model)
    return mean_reward

# 运行优化
study = optuna.create_study(direction="maximize")
study.optimize(objective, n_trials=50)
```

### 5. 高级奖励设计

```python
# 基于行为的奖励塑造
class AdvancedRewardEnv(GoBiggerEnv):
    def calculate_reward(self, old_obs, action, new_obs, info):
        reward = 0
        
        # 基础分数奖励
        if info.get('score_change', 0) > 0:
            reward += info['score_change'] * 0.01
        
        # 移动效率奖励
        if self.is_moving_towards_food(old_obs, action, new_obs):
            reward += 0.1
        
        # 探索奖励
        if self.is_exploring_new_area(new_obs):
            reward += 0.05
        
        # 分裂时机奖励
        if action == 4 and self.is_good_split_timing(new_obs):
            reward += 0.5
        
        return reward
```

---

## 📊 训练监控和日志

### 1. TensorBoard日志

```bash
# 启动TensorBoard监控
tensorboard --logdir=./tensorboard_logs/

# 在浏览器中查看：http://localhost:6006
```

### 2. 自定义日志记录

```python
import logging

# 设置训练日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('training.log'),
        logging.StreamHandler()
    ]
)

class LoggingCallback(BaseCallback):
    def _on_step(self):
        if self.n_calls % 1000 == 0:
            logging.info(f"Step {self.n_calls}: Mean reward = {self.locals.get('rewards', [0])}")
        return True
```

### 3. 模型检查点

```python
# 定期保存模型
class CheckpointCallback(BaseCallback):
    def __init__(self, save_freq, save_path):
        super().__init__()
        self.save_freq = save_freq
        self.save_path = save_path
        
    def _on_step(self):
        if self.n_calls % self.save_freq == 0:
            self.model.save(f"{self.save_path}/model_{self.n_calls}")
        return True

# 使用回调
checkpoint_callback = CheckpointCallback(save_freq=5000, save_path="./checkpoints/")
model.learn(total_timesteps=50000, callback=checkpoint_callback)
```

---

## 🎯 最佳实践总结

### 1. 训练流程建议

1. **从简单开始**: 使用默认参数进行短期训练
2. **观察行为**: 确保智能体基本行为正常
3. **逐步调优**: 一次只改变一个参数
4. **记录实验**: 保存每次实验的配置和结果
5. **定期评估**: 每轮训练后都要测试效果

### 2. 参数调优策略

- **学习率**: 从 1e-4 开始，根据收敛情况调整
- **经验步数**: 内存允许的情况下尽量大
- **训练轮数**: 平衡训练时间和效果
- **奖励函数**: 确保有明确的进度信号

### 3. 调试技巧

- 使用 `verbose=1` 观察训练过程
- 添加调试输出到奖励函数
- 定期运行评估脚本
- 观察智能体的实际游戏行为

---

## 📞 支持和帮助

如果遇到问题，建议：

1. 查看控制台错误信息
2. 检查依赖库是否正确安装
3. 确认C++库构建成功
4. 尝试使用更简单的配置
5. 查看训练日志和TensorBoard

祝您训练愉快！🎮🤖
