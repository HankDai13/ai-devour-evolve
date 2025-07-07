#!/usr/bin/env python3
"""
多智能体GoBigger强化学习训练示例
使用PPO算法训练RL智能体与传统AI ROBOT对战
"""
import sys
import os
from pathlib import Path

# 添加scripts目录到路径
scripts_dir = Path(__file__).parent / "scripts"
sys.path.insert(0, str(scripts_dir))

try:
    from gobigger_gym_env import MultiAgentGoBiggerEnv, MULTI_AGENT_AVAILABLE
except ImportError as e:
    print(f"❌ 导入环境失败: {e}")
    print("请确保已运行 build_multi_agent.bat 编译多智能体模块")
    sys.exit(1)

# 检查多智能体支持
if not MULTI_AGENT_AVAILABLE:
    print("❌ 多智能体功能不可用")
    print("请运行 build_multi_agent.bat 编译多智能体模块")
    sys.exit(1)

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
    
    print(f"Qt environment configured:")
    print(f"  QT_QPA_PLATFORM_PLUGIN_PATH = {os.environ.get('QT_QPA_PLATFORM_PLUGIN_PATH')}")
    print(f"  QT_PLUGIN_PATH = {os.environ.get('QT_PLUGIN_PATH')}")
    print()

# Setup Qt environment before importing any Qt-dependent modules
setup_qt_environment()

def train_multi_agent_ppo():
    """使用PPO训练多智能体环境"""
    print("🚀 启动多智能体PPO训练")
    
    try:
        from stable_baselines3 import PPO
        from stable_baselines3.common.env_util import make_vec_env
        from stable_baselines3.common.callbacks import EvalCallback, CheckpointCallback
        import torch
    except ImportError:
        print("❌ 需要安装 stable-baselines3")
        print("运行: pip install stable-baselines3[extra]")
        return
    
    # 环境配置
    env_config = {
        'max_episode_steps': 2000,      # 更长的episode
        'ai_opponent_count': 3,         # 3个AI对手
        'max_frames': 4000,             # 更多帧数
        'ai_strategies': ['FOOD_HUNTER', 'AGGRESSIVE', 'RANDOM'],
        'max_food_count': 3000,
        'init_food_count': 2500,
    }
    
    # 创建向量化环境
    def make_env():
        return MultiAgentGoBiggerEnv(env_config)
    
    # 使用多个并行环境加速训练
    n_envs = 4
    env = make_vec_env(make_env, n_envs=n_envs)
    
    print(f"✅ 创建了 {n_envs} 个并行多智能体环境")
    print(f"   每个环境包含 1个RL智能体 vs {env_config['ai_opponent_count']}个传统AI")
    
    # PPO配置（针对多智能体优化）
    model = PPO(
        "MlpPolicy",
        env,
        verbose=1,
        # 网络架构
        policy_kwargs=dict(
            net_arch=dict(pi=[512, 512, 256], vf=[512, 512, 256]),
            activation_fn=torch.nn.ReLU,
        ),
        # 学习参数
        learning_rate=3e-4,
        n_steps=2048,           # 每次更新的步数
        batch_size=256,         # 批次大小
        n_epochs=10,            # 每次更新的epoch数
        gamma=0.99,             # 折扣因子
        gae_lambda=0.95,        # GAE lambda
        clip_range=0.2,         # PPO clip range
        ent_coef=0.01,          # 熵系数（鼓励探索）
        vf_coef=0.5,            # 价值函数系数
        max_grad_norm=0.5,      # 梯度裁剪
        # 设备
        device='cuda' if torch.cuda.is_available() else 'cpu',
    )
    
    print(f"✅ PPO模型创建完成，使用设备: {model.device}")
    
    # 创建回调函数
    # 检查点回调
    checkpoint_callback = CheckpointCallback(
        save_freq=10000,  # 每10k步保存一次
        save_path='./models/multi_agent_checkpoints/',
        name_prefix='multi_agent_ppo'
    )
    
    # 评估回调
    eval_env = make_env()
    eval_callback = EvalCallback(
        eval_env,
        best_model_save_path='./models/multi_agent_best/',
        log_path='./logs/multi_agent_eval/',
        eval_freq=5000,    # 每5k步评估一次
        deterministic=True,
        render=False
    )
    
    # 创建目录
    os.makedirs('./models/multi_agent_checkpoints/', exist_ok=True)
    os.makedirs('./models/multi_agent_best/', exist_ok=True)
    os.makedirs('./logs/multi_agent_eval/', exist_ok=True)
    
    print("🏃 开始训练...")
    print("📊 训练统计:")
    print(f"   总训练步数: 100,000")
    print(f"   并行环境数: {n_envs}")
    print(f"   每个环境的AI对手: {env_config['ai_opponent_count']}")
    print(f"   观察空间维度: {env.observation_space.shape}")
    print(f"   动作空间维度: {env.action_space.shape}")
    
    # 训练模型
    model.learn(
        total_timesteps=100000,  # 总训练步数
        callback=[checkpoint_callback, eval_callback],
        progress_bar=True
    )
    
    # 保存最终模型
    model.save("./models/multi_agent_ppo_final")
    print("✅ 训练完成，模型已保存")
    
    # 清理环境
    env.close()
    eval_env.close()
    
    return model

def test_trained_model():
    """测试训练好的模型"""
    print("🧪 测试训练好的多智能体模型")
    
    try:
        from stable_baselines3 import PPO
    except ImportError:
        print("❌ 需要安装 stable-baselines3")
        return
    
    # 检查模型文件
    model_path = "./models/multi_agent_ppo_final.zip"
    if not os.path.exists(model_path):
        print(f"❌ 未找到模型文件: {model_path}")
        print("请先运行训练")
        return
    
    # 加载模型
    model = PPO.load(model_path)
    print(f"✅ 加载模型: {model_path}")
    
    # 创建测试环境
    env_config = {
        'max_episode_steps': 1000,
        'ai_opponent_count': 3,
        'ai_strategies': ['FOOD_HUNTER', 'AGGRESSIVE', 'RANDOM'],
        'debug_mode': True,  # 启用调试模式
    }
    env = MultiAgentGoBiggerEnv(env_config)
    
    # 运行测试episodes
    n_test_episodes = 5
    total_rewards = []
    final_ranks = []
    
    for episode in range(n_test_episodes):
        print(f"\n🎮 测试Episode {episode + 1}/{n_test_episodes}")
        
        obs = env.reset()
        episode_reward = 0
        step = 0
        
        while True:
            # 使用训练好的策略
            action, _states = model.predict(obs, deterministic=True)
            obs, reward, done, info = env.step(action)
            episode_reward += reward
            step += 1
            
            # 每50步显示一次状态
            if step % 50 == 0:
                env.render()
            
            if done:
                total_rewards.append(episode_reward)
                final_rank = info.get('final_rank', 1)
                final_ranks.append(final_rank)
                
                print(f"📊 Episode {episode + 1} 结果:")
                print(f"   总奖励: {episode_reward:.2f}")
                print(f"   最终排名: {final_rank}/{info.get('total_teams', 1)}")
                print(f"   分数变化: {info.get('score_delta', 0):+.1f}")
                print(f"   存活AI: {info.get('ai_opponents_alive', 0)}")
                break
    
    # 统计结果
    print(f"\n📈 测试结果统计 ({n_test_episodes} episodes):")
    print(f"   平均奖励: {sum(total_rewards)/len(total_rewards):.2f}")
    print(f"   平均排名: {sum(final_ranks)/len(final_ranks):.1f}")
    print(f"   最佳排名: {min(final_ranks)}")
    print(f"   排名1的次数: {final_ranks.count(1)}/{n_test_episodes}")
    
    env.close()

def main():
    """主函数"""
    print("🤖 多智能体GoBigger强化学习训练系统")
    print("="*50)
    
    if len(sys.argv) > 1:
        if sys.argv[1] == "--train":
            train_multi_agent_ppo()
        elif sys.argv[1] == "--test":
            test_trained_model()
        else:
            print("❌ 未知参数")
            print("使用方法:")
            print("  python train_multi_agent.py --train   # 训练模型")
            print("  python train_multi_agent.py --test    # 测试模型")
    else:
        print("🎯 使用方法:")
        print("  python train_multi_agent.py --train   # 训练RL智能体与传统AI对战")
        print("  python train_multi_agent.py --test    # 测试训练好的模型")
        print()
        print("🤖 多智能体特点:")
        print("  • RL智能体 vs 3个传统AI ROBOT")
        print("  • AI策略: FOOD_HUNTER, AGGRESSIVE, RANDOM")  
        print("  • 团队排名作为奖励信号")
        print("  • 增强的观察空间（450维）")
        print("  • 支持并行训练加速")

if __name__ == "__main__":
    main()
