#!/usr/bin/env python3
"""
使用增强奖励系统的训练脚本
=====================================

此脚本展示如何使用增强奖励系统来训练GoBigger AI智能体。
增强奖励系统提供更密集、更有意义的奖励信号，帮助智能体更好地学习游戏策略。

特性:
- 增强奖励系统：非线性分数奖励、效率奖励、探索奖励等
- 实时奖励组件分析
- 详细训练日志
- 可配置的奖励权重

作者: AI Assistant
日期: 2024年
"""

import sys
import os
import time
import numpy as np
from datetime import datetime

# 添加当前目录到Python路径
current_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(current_dir)

try:
    from rich.console import Console
    from rich.table import Table
    from rich.progress import Progress, TextColumn, BarColumn, TimeElapsedColumn, TimeRemainingColumn
    from rich.panel import Panel
    from rich.layout import Layout
    from rich.text import Text
    RICH_AVAILABLE = True
except ImportError:
    RICH_AVAILABLE = False

# 导入我们的模块
from gobigger_gym_env import GoBiggerEnv
from enhanced_reward_system import EnhancedRewardCalculator

def create_enhanced_config(enable_enhanced_reward=True):
    """创建启用增强奖励的环境配置"""
    config = {
        'max_episode_steps': 2000,
        'use_enhanced_reward': enable_enhanced_reward,
        'enhanced_reward_weights': {
            'score_growth': 2.0,        # 提高分数增长奖励权重
            'efficiency': 1.5,          # 提高效率奖励权重
            'exploration': 0.8,         # 探索奖励权重
            'strategic_split': 2.0,     # 增强战略分裂奖励
            'food_density': 1.0,        # 食物密度奖励权重
            'survival': 0.02,           # 生存奖励权重
            'time_penalty': -0.001,     # 时间惩罚权重
            'death_penalty': -20.0,     # 增强死亡惩罚
        }
    }
    return config

def train_with_enhanced_rewards(total_episodes=10, save_interval=5):
    """使用增强奖励系统训练智能体"""
    
    if RICH_AVAILABLE:
        console = Console()
        console.print(Panel.fit(
            "[bold green]🚀 增强奖励系统训练[/bold green]\n"
            "[cyan]启用密集奖励信号和智能策略奖励[/cyan]",
            border_style="green"
        ))
    else:
        print("=" * 60)
        print("🚀 增强奖励系统训练")
        print("启用密集奖励信号和智能策略奖励")
        print("=" * 60)
    
    # 创建环境（启用增强奖励）
    config = create_enhanced_config(enable_enhanced_reward=True)
    env = GoBiggerEnv(config)
    
    # 训练统计
    episode_scores = []
    episode_rewards = []
    episode_lengths = []
    reward_component_stats = {}
    
    # 创建进度条（如果有rich）
    if RICH_AVAILABLE:
        progress = Progress(
            TextColumn("[progress.description]{task.description}"),
            BarColumn(),
            TextColumn("[progress.percentage]{task.percentage:>3.0f}%"),
            TimeElapsedColumn(),
            TimeRemainingColumn(),
            console=console
        )
        task = progress.add_task("训练进度", total=total_episodes)
        progress.start()
    
    try:
        for episode in range(total_episodes):
            obs = env.reset()
            episode_reward = 0.0
            step_count = 0
            episode_start_time = time.time()
            
            # 记录episode开始时的分数
            if env.current_obs and env.current_obs.player_states:
                initial_score = list(env.current_obs.player_states.values())[0].score
            else:
                initial_score = 0.0
            
            # Episode循环
            while True:
                # 生成随机动作（实际训练中这里会是智能体的策略）
                action = env.action_space.sample()
                
                # 执行动作
                obs, reward, terminated, truncated, info = env.step(action)
                episode_reward += reward
                step_count += 1
                
                # 收集奖励组件统计（如果使用增强奖励）
                if hasattr(env, 'reward_components_history') and env.reward_components_history:
                    latest_components = env.reward_components_history[-1]['components']
                    for component, value in latest_components.items():
                        if component not in reward_component_stats:
                            reward_component_stats[component] = []
                        reward_component_stats[component].append(value * env.config.get('enhanced_reward_weights', {}).get(component, 1.0))
                
                # 检查是否结束
                if terminated or truncated:
                    break
            
            # 记录episode统计
            final_score = info.get('final_score', initial_score)
            score_delta = final_score - initial_score
            episode_duration = time.time() - episode_start_time
            
            episode_scores.append(final_score)
            episode_rewards.append(episode_reward)
            episode_lengths.append(step_count)
            
            # 显示episode结果
            if RICH_AVAILABLE:
                # 更新进度条
                progress.update(task, advance=1)
                
                # 创建详细信息表
                table = Table(title=f"Episode {episode + 1} 结果")
                table.add_column("指标", style="cyan")
                table.add_column("数值", style="green")
                
                table.add_row("最终分数", f"{final_score:.2f}")
                table.add_row("分数增长", f"{score_delta:+.2f}")
                table.add_row("总奖励", f"{episode_reward:.4f}")
                table.add_row("步数", str(step_count))
                table.add_row("持续时间", f"{episode_duration:.2f}s")
                
                # 如果有奖励组件统计，显示最近的平均值
                if reward_component_stats:
                    table.add_row("", "")  # 分隔线
                    table.add_row("[bold]奖励组件 (最近10步平均)[/bold]", "")
                    for component, values in reward_component_stats.items():
                        if values:
                            recent_avg = np.mean(values[-10:]) if len(values) >= 10 else np.mean(values)
                            table.add_row(f"  {component}", f"{recent_avg:.4f}")
                
                console.print(table)
                
            else:
                print(f"\nEpisode {episode + 1}/{total_episodes}:")
                print(f"  最终分数: {final_score:.2f} (增长: {score_delta:+.2f})")
                print(f"  总奖励: {episode_reward:.4f}")
                print(f"  步数: {step_count}, 持续时间: {episode_duration:.2f}s")
                
                # 显示奖励组件统计
                if reward_component_stats:
                    print(f"  奖励组件 (最近10步平均):")
                    for component, values in reward_component_stats.items():
                        if values:
                            recent_avg = np.mean(values[-10:]) if len(values) >= 10 else np.mean(values)
                            print(f"    {component}: {recent_avg:.4f}")
            
            # 保存模型检查点（每隔一定episode）
            if (episode + 1) % save_interval == 0:
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                checkpoint_info = {
                    'episode': episode + 1,
                    'avg_score': np.mean(episode_scores[-save_interval:]),
                    'avg_reward': np.mean(episode_rewards[-save_interval:]),
                    'timestamp': timestamp
                }
                
                if RICH_AVAILABLE:
                    console.print(f"[yellow]💾 检查点 {episode + 1}: 平均分数 {checkpoint_info['avg_score']:.2f}[/yellow]")
                else:
                    print(f"💾 检查点 {episode + 1}: 平均分数 {checkpoint_info['avg_score']:.2f}")
        
        # 训练完成总结
        if episode_scores:
            avg_score = np.mean(episode_scores)
            max_score = np.max(episode_scores)
            avg_reward = np.mean(episode_rewards)
            avg_length = np.mean(episode_lengths)
            
            if RICH_AVAILABLE:
                summary_table = Table(title="🎯 训练总结", border_style="green")
                summary_table.add_column("指标", style="cyan")
                summary_table.add_column("数值", style="green")
                
                summary_table.add_row("总Episodes", str(total_episodes))
                summary_table.add_row("平均分数", f"{avg_score:.2f}")
                summary_table.add_row("最高分数", f"{max_score:.2f}")
                summary_table.add_row("平均奖励", f"{avg_reward:.4f}")
                summary_table.add_row("平均步数", f"{avg_length:.1f}")
                
                # 奖励组件总结
                if reward_component_stats:
                    summary_table.add_row("", "")  # 分隔线
                    summary_table.add_row("[bold]奖励组件总平均[/bold]", "")
                    for component, values in reward_component_stats.items():
                        if values:
                            overall_avg = np.mean(values)
                            summary_table.add_row(f"  {component}", f"{overall_avg:.4f}")
                
                console.print(summary_table)
                progress.stop()
                
            else:
                print(f"\n{'='*60}")
                print("🎯 训练总结")
                print(f"{'='*60}")
                print(f"总Episodes: {total_episodes}")
                print(f"平均分数: {avg_score:.2f}")
                print(f"最高分数: {max_score:.2f}")
                print(f"平均奖励: {avg_reward:.4f}")
                print(f"平均步数: {avg_length:.1f}")
                
                if reward_component_stats:
                    print(f"\n奖励组件总平均:")
                    for component, values in reward_component_stats.items():
                        if values:
                            overall_avg = np.mean(values)
                            print(f"  {component}: {overall_avg:.4f}")
        
    except KeyboardInterrupt:
        if RICH_AVAILABLE:
            progress.stop()
            console.print("[red]❌ 训练被用户中断[/red]")
        else:
            print("\n❌ 训练被用户中断")
    
    finally:
        env.close()

def compare_reward_systems(episodes_per_system=5):
    """比较标准奖励和增强奖励系统的性能"""
    
    if RICH_AVAILABLE:
        console = Console()
        console.print(Panel.fit(
            "[bold blue]⚖️  奖励系统对比测试[/bold blue]\n"
            "[cyan]比较标准奖励 vs 增强奖励系统的表现[/cyan]",
            border_style="blue"
        ))
    else:
        print("=" * 60)
        print("⚖️  奖励系统对比测试")
        print("比较标准奖励 vs 增强奖励系统的表现")
        print("=" * 60)
    
    results = {}
    
    for system_name, use_enhanced in [("标准奖励", False), ("增强奖励", True)]:
        if RICH_AVAILABLE:
            console.print(f"\n[bold]{system_name}系统测试[/bold]")
        else:
            print(f"\n{system_name}系统测试:")
        
        config = create_enhanced_config(enable_enhanced_reward=use_enhanced)
        env = GoBiggerEnv(config)
        
        scores = []
        rewards = []
        lengths = []
        
        for episode in range(episodes_per_system):
            obs = env.reset()
            episode_reward = 0.0
            step_count = 0
            
            if env.current_obs and env.current_obs.player_states:
                initial_score = list(env.current_obs.player_states.values())[0].score
            else:
                initial_score = 0.0
            
            while True:
                action = env.action_space.sample()
                obs, reward, terminated, truncated, info = env.step(action)
                episode_reward += reward
                step_count += 1
                
                if terminated or truncated:
                    break
            
            final_score = info.get('final_score', initial_score)
            scores.append(final_score)
            rewards.append(episode_reward)
            lengths.append(step_count)
            
            if RICH_AVAILABLE:
                console.print(f"  Episode {episode + 1}: 分数 {final_score:.2f}, 奖励 {episode_reward:.4f}")
            else:
                print(f"  Episode {episode + 1}: 分数 {final_score:.2f}, 奖励 {episode_reward:.4f}")
        
        results[system_name] = {
            'avg_score': np.mean(scores),
            'max_score': np.max(scores),
            'avg_reward': np.mean(rewards),
            'avg_length': np.mean(lengths),
            'std_score': np.std(scores)
        }
        
        env.close()
    
    # 显示比较结果
    if RICH_AVAILABLE:
        comparison_table = Table(title="📊 奖励系统对比结果", border_style="yellow")
        comparison_table.add_column("指标", style="cyan")
        comparison_table.add_column("标准奖励", style="red")
        comparison_table.add_column("增强奖励", style="green")
        comparison_table.add_column("改进", style="yellow")
        
        for metric in ['avg_score', 'max_score', 'avg_reward', 'avg_length', 'std_score']:
            standard = results["标准奖励"][metric]
            enhanced = results["增强奖励"][metric]
            
            if metric in ['avg_score', 'max_score', 'avg_reward']:
                improvement = f"{((enhanced - standard) / abs(standard) * 100):+.1f}%" if standard != 0 else "N/A"
            else:
                improvement = f"{((standard - enhanced) / abs(standard) * 100):+.1f}%" if standard != 0 else "N/A"
            
            metric_name = {
                'avg_score': '平均分数',
                'max_score': '最高分数', 
                'avg_reward': '平均奖励',
                'avg_length': '平均步数',
                'std_score': '分数标准差'
            }[metric]
            
            comparison_table.add_row(
                metric_name,
                f"{standard:.3f}",
                f"{enhanced:.3f}",
                improvement
            )
        
        console.print(comparison_table)
        
    else:
        print(f"\n{'='*60}")
        print("📊 奖励系统对比结果")
        print(f"{'='*60}")
        print(f"{'指标':<15} {'标准奖励':<15} {'增强奖励':<15} {'改进':<10}")
        print("-" * 60)
        
        for metric in ['avg_score', 'max_score', 'avg_reward', 'avg_length', 'std_score']:
            standard = results["标准奖励"][metric]
            enhanced = results["增强奖励"][metric]
            
            if metric in ['avg_score', 'max_score', 'avg_reward']:
                improvement = f"{((enhanced - standard) / abs(standard) * 100):+.1f}%" if standard != 0 else "N/A"
            else:
                improvement = f"{((standard - enhanced) / abs(standard) * 100):+.1f}%" if standard != 0 else "N/A"
            
            metric_name = {
                'avg_score': '平均分数',
                'max_score': '最高分数',
                'avg_reward': '平均奖励', 
                'avg_length': '平均步数',
                'std_score': '分数标准差'
            }[metric]
            
            print(f"{metric_name:<15} {standard:<15.3f} {enhanced:<15.3f} {improvement:<10}")

def main():
    """主函数"""
    print("🎮 GoBigger 增强奖励系统训练工具")
    print("1. 增强奖励训练 (推荐)")
    print("2. 奖励系统对比测试")
    print("3. 退出")
    
    try:
        choice = input("\n请选择操作 (1-3): ").strip()
        
        if choice == "1":
            episodes = input("请输入训练Episodes数量 (默认: 10): ").strip()
            episodes = int(episodes) if episodes.isdigit() else 10
            train_with_enhanced_rewards(total_episodes=episodes)
            
        elif choice == "2":
            episodes = input("请输入每个系统的测试Episodes数量 (默认: 5): ").strip()
            episodes = int(episodes) if episodes.isdigit() else 5
            compare_reward_systems(episodes_per_system=episodes)
            
        elif choice == "3":
            print("👋 再见!")
            
        else:
            print("❌ 无效选择")
            
    except KeyboardInterrupt:
        print("\n\n👋 程序被用户中断，再见!")
    except Exception as e:
        print(f"\n❌ 发生错误: {e}")

if __name__ == "__main__":
    main()
