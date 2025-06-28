#!/usr/bin/env python3
"""
简化版美化训练界面
不依赖复杂的rich功能，使用基本的控制台美化
"""
import sys
import os
from pathlib import Path
import numpy as np
import time
from collections import deque

# 路径设置
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

import gobigger_env
from gobigger_gym_env import GoBiggerEnv

try:
    from stable_baselines3 import PPO
    from stable_baselines3.common.env_util import make_vec_env
    from stable_baselines3.common.vec_env import DummyVecEnv
    from stable_baselines3.common.callbacks import BaseCallback
    STABLE_BASELINES_AVAILABLE = True
except ImportError:
    print("❌ 需要安装 stable-baselines3")
    exit(1)

class SimpleTrainingDisplay:
    """简化版训练显示器"""
    
    def __init__(self, total_timesteps):
        self.total_timesteps = total_timesteps
        self.start_time = time.time()
        
        # 训练统计
        self.stats = {
            'ep_len_mean': 0,
            'ep_rew_mean': 0,
            'ep_score_mean': 0,
            'episodes_completed': 0,
            'best_score': 0,
            'current_timesteps': 0,
            'fps': 0,
            'time_elapsed': 0
        }
        
        self.episode_scores = deque(maxlen=100)
        
    def clear_screen(self):
        """清屏"""
        os.system('cls' if os.name == 'nt' else 'clear')
    
    def create_progress_bar(self, current, total, width=50):
        """创建进度条"""
        percent = current / total
        filled = int(width * percent)
        bar = '█' * filled + '░' * (width - filled)
        return f"[{bar}] {percent*100:.1f}%"
    
    def update_stats(self, timesteps, episode_info=None, logger_data=None):
        """更新统计信息"""
        self.stats['current_timesteps'] = timesteps
        self.stats['time_elapsed'] = time.time() - self.start_time
        
        if self.stats['time_elapsed'] > 0:
            self.stats['fps'] = timesteps / self.stats['time_elapsed']
        
        # 更新episode信息
        if episode_info:
            if 'final_score' in episode_info:
                score = episode_info['final_score']
                self.episode_scores.append(score)
                self.stats['episodes_completed'] += 1
                self.stats['best_score'] = max(self.stats['best_score'], score)
        
        # 更新平均值
        if len(self.episode_scores) > 0:
            self.stats['ep_score_mean'] = np.mean(self.episode_scores)
        
        # 从logger更新训练指标
        if logger_data:
            if 'rollout/ep_len_mean' in logger_data:
                self.stats['ep_len_mean'] = logger_data['rollout/ep_len_mean']
            if 'rollout/ep_rew_mean' in logger_data:
                self.stats['ep_rew_mean'] = logger_data['rollout/ep_rew_mean']
    
    def display_training_status(self):
        """显示训练状态"""
        self.clear_screen()
        
        print("🤖 GoBigger Reinforcement Learning Training")
        print("=" * 60)
        print()
        
        # 进度条
        progress_bar = self.create_progress_bar(self.stats['current_timesteps'], self.total_timesteps)
        print(f"📈 Training Progress: {progress_bar}")
        print(f"   Steps: {self.stats['current_timesteps']:,}/{self.total_timesteps:,}")
        print()
        
        # 训练统计表格
        print("📊 Training Statistics:")
        print("┌─────────────────┬─────────────────┐")
        print(f"│ Episodes        │ {self.stats['episodes_completed']:>15,} │")
        print(f"│ Best Score      │ {self.stats['best_score']:>15.0f} │")
        print(f"│ Avg Score       │ {self.stats['ep_score_mean']:>15.0f} │")
        print(f"│ Avg Reward      │ {self.stats['ep_rew_mean']:>15.2f} │")
        print(f"│ Avg Length      │ {self.stats['ep_len_mean']:>15.0f} │")
        print("├─────────────────┼─────────────────┤")
        print(f"│ FPS             │ {self.stats['fps']:>15.0f} │")
        print(f"│ Time Elapsed    │ {self.stats['time_elapsed']:>15.0f}s │")
        print("└─────────────────┴─────────────────┘")
        print()
        
        # 最新信息
        if len(self.episode_scores) > 0:
            latest_score = self.episode_scores[-1]
            print(f"🎯 Latest Episode Score: {latest_score:.0f}")
        
        # 预估剩余时间
        if self.stats['fps'] > 0:
            remaining_steps = self.total_timesteps - self.stats['current_timesteps']
            eta_seconds = remaining_steps / self.stats['fps']
            eta_minutes = eta_seconds / 60
            print(f"⏱️  Estimated Time Remaining: {eta_minutes:.1f} minutes")
        
        print()
        print("💡 Tip: 按 Ctrl+C 可以中断训练")

class SimpleTrainingCallback(BaseCallback):
    """简化版训练回调"""
    
    def __init__(self, display, update_interval=5, verbose=1):
        super().__init__(verbose)
        self.display = display
        self.update_interval = update_interval
        self.last_update = 0
        
    def _on_step(self) -> bool:
        current_time = time.time()
        
        # 处理episode结束信息
        episode_info = None
        for info in self.locals['infos']:
            if 'final_score' in info:
                episode_info = info
                # 立即显示episode结束信息
                score = info['final_score']
                score_delta = info.get('score_delta', 0)
                print(f"\r🎯 Episode 结束 - 分数: {score:.0f} (变化: {score_delta:+.0f})")
                break
        
        # 获取logger数据
        logger_data = None
        if hasattr(self.model, 'logger') and hasattr(self.model.logger, 'name_to_value'):
            logger_data = self.model.logger.name_to_value
        
        # 更新统计
        self.display.update_stats(self.num_timesteps, episode_info, logger_data)
        
        # 定期更新界面
        if current_time - self.last_update > self.update_interval:
            self.display.display_training_status()
            self.last_update = current_time
        
        return True

def create_env(config=None):
    """创建训练环境"""
    default_config = {
        'max_episode_steps': 1500,
    }
    if config:
        default_config.update(config)
    
    return GoBiggerEnv(default_config)

def train_with_simple_ui(total_timesteps=20000):
    """使用简化界面训练"""
    print("🚀 启动简化美化界面训练")
    print()
    
    # 创建显示器
    display = SimpleTrainingDisplay(total_timesteps)
    
    # 创建环境和模型
    env = make_vec_env(lambda: create_env(), n_envs=1, vec_env_cls=DummyVecEnv)
    
    model = PPO(
        "MlpPolicy", 
        env,
        learning_rate=3e-4,
        n_steps=1024,
        batch_size=64,
        n_epochs=10,
        gamma=0.99,
        verbose=0,  # 静默模式
        tensorboard_log="./tensorboard_logs/"
    )
    
    # 创建回调
    callback = SimpleTrainingCallback(display, update_interval=3)
    
    try:
        # 开始训练
        model.learn(
            total_timesteps=total_timesteps,
            callback=callback,
            tb_log_name="PPO_gobigger_simple"
        )
        
        # 最后显示一次完整状态
        display.display_training_status()
        
    except KeyboardInterrupt:
        print("\n⚠️ 训练被用户中断")
        display.display_training_status()
    
    # 保存模型
    os.makedirs("models", exist_ok=True)
    model_path = "models/PPO_gobigger_simple.zip"
    model.save(model_path)
    
    print(f"\n✅ 模型已保存: {model_path}")
    print(f"📊 最终统计:")
    print(f"  - 完成Episodes: {display.stats['episodes_completed']}")
    print(f"  - 最佳分数: {display.stats['best_score']:.0f}")
    print(f"  - 平均分数: {display.stats['ep_score_mean']:.0f}")
    print(f"  - 训练时长: {display.stats['time_elapsed']:.0f}秒")
    
    return model

def main():
    """主函数"""
    print("🎮 GoBigger 简化美化界面训练器")
    print("=" * 60)
    print()
    print("训练配置:")
    print("  - 算法: PPO")
    print("  - 训练步数: 20,000")
    print("  - Episode长度: 1,500")
    print("  - 界面: 简化美化界面")
    print()
    
    confirm = input("开始训练? (y/n): ").lower().strip()
    if confirm in ['y', 'yes', '是']:
        try:
            train_with_simple_ui(total_timesteps=20000)
        except Exception as e:
            print(f"\n❌ 训练出错: {e}")
    else:
        print("👋 已取消训练")

if __name__ == "__main__":
    main()
