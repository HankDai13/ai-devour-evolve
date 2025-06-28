#!/usr/bin/env python3
"""
使用Rich界面的GoBigger强化学习训练器
美化版训练界面，带进度条和表格显示
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
    from rich.console import Console
    from rich.table import Table
    from rich.progress import Progress, BarColumn, TextColumn, TimeElapsedColumn, TimeRemainingColumn
    from rich.panel import Panel
    from rich.text import Text
    from rich.live import Live
    from rich.layout import Layout
    RICH_AVAILABLE = True
except ImportError:
    print("❌ 需要安装 rich 库: pip install rich")
    exit(1)

try:
    from stable_baselines3 import PPO
    from stable_baselines3.common.env_util import make_vec_env
    from stable_baselines3.common.vec_env import DummyVecEnv
    from stable_baselines3.common.callbacks import BaseCallback
    STABLE_BASELINES_AVAILABLE = True
except ImportError:
    print("❌ 需要安装 stable-baselines3")
    exit(1)

class RichTrainingDisplay:
    """Rich界面训练显示器"""
    
    def __init__(self, total_timesteps):
        self.console = Console()
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
        self.last_update = 0
        
    def create_layout(self):
        """创建界面布局"""
        layout = Layout()
        layout.split_column(
            Layout(name="header", size=3),
            Layout(name="stats", size=12),
            Layout(name="progress", size=8),
            Layout(name="footer", size=3)
        )
        return layout
        
    def create_stats_table(self):
        """创建统计表格"""
        table = Table(title="🤖 GoBigger RL Training Status", show_header=True, header_style="bold blue")
        table.add_column("Category", style="cyan", width=12)
        table.add_column("Metric", style="blue", width=15)
        table.add_column("Value", style="magenta", width=15)
        
        # Episode统计
        table.add_row("rollout/", "ep_len_mean", f"{self.stats['ep_len_mean']:.0f}")
        table.add_row("", "ep_rew_mean", f"{self.stats['ep_rew_mean']:.2f}")
        table.add_row("", "ep_score_mean", f"{self.stats['ep_score_mean']:.0f}")
        table.add_row("", "best_score", f"{self.stats['best_score']:.0f}")
        
        # 时间统计
        table.add_row("time/", "fps", f"{self.stats['fps']:.0f}")
        table.add_row("", "elapsed", f"{self.stats['time_elapsed']:.0f}s")
        table.add_row("", "episodes", f"{self.stats['episodes_completed']}")
        
        # 进度统计
        progress_percent = (self.stats['current_timesteps'] / self.total_timesteps) * 100
        table.add_row("progress/", "timesteps", f"{self.stats['current_timesteps']:,}/{self.total_timesteps:,}")
        table.add_row("", "percent", f"{progress_percent:.1f}%")
        
        return table
    
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
    
    def update_display(self, layout):
        """更新显示内容"""
        # 头部
        header_text = Text("🎮 GoBigger Reinforcement Learning Training", style="bold green")
        layout["header"].update(Panel(header_text, style="green"))
        
        # 统计表格
        stats_table = self.create_stats_table()
        layout["stats"].update(Panel(stats_table, title="📊 Training Metrics", style="blue"))
        
        # 进度条
        progress_percent = (self.stats['current_timesteps'] / self.total_timesteps) * 100
        progress_text = f"Training Progress: {self.stats['current_timesteps']:,}/{self.total_timesteps:,} steps ({progress_percent:.1f}%)"
        
        # 创建进度条
        with Progress(
            TextColumn("[progress.description]{task.description}"),
            BarColumn(bar_width=60),
            TextColumn("[progress.percentage]{task.percentage:>3.0f}%"),
            TimeElapsedColumn(),
            TimeRemainingColumn(),
        ) as progress:
            task = progress.add_task("Training...", total=self.total_timesteps)
            progress.update(task, completed=self.stats['current_timesteps'])
            layout["progress"].update(Panel(progress, title="📈 Progress", style="green"))
        
        # 底部信息
        if len(self.episode_scores) > 0:
            latest_score = self.episode_scores[-1]
            footer_text = f"🎯 Latest Episode Score: {latest_score:.0f} | 🏆 Best Score: {self.stats['best_score']:.0f} | ⚡ FPS: {self.stats['fps']:.0f}"
        else:
            footer_text = f"⚡ FPS: {self.stats['fps']:.0f} | 🕒 Elapsed: {self.stats['time_elapsed']:.0f}s"
        
        layout["footer"].update(Panel(Text(footer_text, style="yellow"), style="yellow"))

class RichTrainingCallback(BaseCallback):
    """带Rich界面的训练回调"""
    
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
                break
        
        # 获取logger数据
        logger_data = None
        if hasattr(self.model, 'logger') and hasattr(self.model.logger, 'name_to_value'):
            logger_data = self.model.logger.name_to_value
        
        # 更新统计
        self.display.update_stats(self.num_timesteps, episode_info, logger_data)
        
        return True

def create_env(config=None):
    """创建训练环境"""
    default_config = {
        'max_episode_steps': 1500,
    }
    if config:
        default_config.update(config)
    
    return GoBiggerEnv(default_config)

def train_with_rich_interface(total_timesteps=20000):
    """使用Rich界面训练"""
    console = Console()
    console.print("🚀 启动Rich界面训练", style="bold green")
    
    # 创建显示器
    display = RichTrainingDisplay(total_timesteps)
    layout = display.create_layout()
    
    # 创建环境和模型
    env = make_vec_env(lambda: create_env(), n_envs=1, vec_env_cls=DummyVecEnv)
    
    model = PPO(
        "MlpPolicy", 
        env,
        learning_rate=3e-4,
        n_steps=1024,  # 减少步数，更频繁更新
        batch_size=64,
        n_epochs=10,
        gamma=0.99,
        verbose=0,  # 静默模式
        tensorboard_log="./tensorboard_logs/"
    )
    
    # 创建回调
    callback = RichTrainingCallback(display, update_interval=2)
    
    # 开始训练，带实时界面
    with Live(layout, refresh_per_second=2, console=console) as live:
        def train():
            model.learn(
                total_timesteps=total_timesteps,
                callback=callback,
                tb_log_name="PPO_gobigger_rich"
            )
        
        # 在单独线程中运行训练
        import threading
        
        training_complete = threading.Event()
        training_error = None
        
        def training_thread():
            nonlocal training_error
            try:
                train()
            except Exception as e:
                training_error = e
            finally:
                training_complete.set()
        
        # 启动训练线程
        train_thread = threading.Thread(target=training_thread)
        train_thread.start()
        
        # 实时更新界面
        while not training_complete.is_set():
            display.update_display(layout)
            time.sleep(0.5)  # 每500ms更新一次
        
        # 最后更新一次
        display.update_display(layout)
        
        # 等待训练完成
        train_thread.join()
        
        if training_error:
            raise training_error
    
    # 保存模型
    os.makedirs("models", exist_ok=True)
    model_path = "models/PPO_gobigger_rich.zip"
    model.save(model_path)
    
    console.print(f"\n✅ 训练完成！模型已保存: {model_path}", style="bold green")
    console.print(f"📊 最终统计:", style="bold blue")
    console.print(f"  - 完成Episodes: {display.stats['episodes_completed']}")
    console.print(f"  - 最佳分数: {display.stats['best_score']:.0f}")
    console.print(f"  - 平均分数: {display.stats['ep_score_mean']:.0f}")
    console.print(f"  - 训练时长: {display.stats['time_elapsed']:.0f}秒")
    
    return model

def main():
    """主函数"""
    console = Console()
    console.print("🎮 GoBigger Rich界面训练器", style="bold cyan")
    console.print("=" * 60, style="cyan")
    
    print("训练配置:")
    print("  - 算法: PPO")
    print("  - 训练步数: 20,000")
    print("  - Episode长度: 1,500")
    print("  - 界面: Rich美化界面")
    print()
    
    confirm = input("开始训练? (y/n): ").lower().strip()
    if confirm in ['y', 'yes', '是']:
        try:
            train_with_rich_interface(total_timesteps=20000)
        except KeyboardInterrupt:
            console.print("\n⚠️ 训练被用户中断", style="yellow")
        except Exception as e:
            console.print(f"\n❌ 训练出错: {e}", style="red")
    else:
        console.print("👋 已取消训练", style="yellow")

if __name__ == "__main__":
    main()
