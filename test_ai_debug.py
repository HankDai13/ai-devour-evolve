#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AI分数调试专用脚本
深度检查AI为何分数不增长
"""

import sys
import os
import time

# 设置Qt环境
os.environ['QT_QPA_PLATFORM_PLUGIN_PATH'] = r'D:\qt\6.9.1\msvc2022_64\plugins\platforms'

# 确保Python模块路径正确
workspace_root = r"d:\Coding\Projects\ai-devour-evolve"
python_dir = os.path.join(workspace_root, "python")
sys.path.insert(0, python_dir)

try:
    import gobigger_multi_env
    print("✅ 多智能体模块导入成功")
except ImportError as e:
    print(f"❌ 多智能体模块导入失败: {e}")
    sys.exit(1)

def test_ai_behavior():
    """测试AI行为和分数变化"""
    print("=" * 60)
    print("🔍 AI行为调试测试")
    print("=" * 60)
    
    # 创建多智能体环境
    config = gobigger_multi_env.MultiAgentConfig()
    engine = gobigger_multi_env.MultiAgentGameEngine(config)
    print("🤖 多智能体环境创建成功")
    
    # 重置环境
    obs = engine.reset()
    print("🔄 环境重置完成")
    
    # 初始状态
    reward_info = engine.get_reward_info()
    print(f"\n📊 初始状态:")
    print(f"   RL智能体分数: {reward_info.get('rl_score', 0)}")
    print(f"   AI分数: {[reward_info.get(f'ai_{i}_score', 0) for i in range(3)]}")
    print(f"   团队排名: {reward_info.get('team_ranking', [])}")
    
    # 运行多步，观察AI分数变化
    print(f"\n🎮 开始{30}步测试，观察AI分数变化...")
    
    for step in range(30):
        # RL智能体随机移动
        action = {
            "rl_agent": [0.1, 0.1, 0]  # 小幅随机移动
        }
        
        # 执行一步
        obs = engine.step(action)
        
        # 获取奖励信息
        reward_info = engine.get_reward_info()
        rl_score = reward_info.get('rl_score', 0)
        ai_scores = [reward_info.get(f'ai_{i}_score', 0) for i in range(3)]
        team_ranking = reward_info.get('team_ranking', [])
        
        # 每5步输出一次状态
        if step % 5 == 4:
            print(f"\n📊 第{step+1}步状态:")
            print(f"   RL智能体分数: {rl_score}")
            print(f"   AI分数: {ai_scores}")
            print(f"   团队排名: {team_ranking}")
            print(f"   AI分数变化: {[ai_scores[i] - 1000 for i in range(3)]}")
            
            # 检查AI分数是否有任何增长
            ai_progress = [score > 1000 for score in ai_scores]
            if any(ai_progress):
                print(f"   ✅ AI有进展: {ai_progress}")
            else:
                print(f"   ❌ AI分数未增长")
        
        # 短暂延迟以让AI有时间决策
        time.sleep(0.1)
    
    # 最终总结
    final_reward_info = engine.get_reward_info()
    final_ai_scores = [final_reward_info.get(f'ai_{i}_score', 0) for i in range(3)]
    
    print(f"\n" + "=" * 60)
    print(f"🏁 测试完成总结:")
    print(f"   RL最终分数: {final_reward_info.get('rl_score', 0)}")
    print(f"   AI最终分数: {final_ai_scores}")
    print(f"   AI分数增长: {[score - 1000 for score in final_ai_scores]}")
    
    # 判断AI是否正常工作
    ai_working = any(score > 1000 for score in final_ai_scores)
    if ai_working:
        print(f"   ✅ AI正常工作，有分数增长")
    else:
        print(f"   ❌ AI分数未增长，存在问题")
        print(f"   🔧 可能原因:")
        print(f"      - AI决策未被调用")
        print(f"      - AI移动逻辑有问题")
        print(f"      - 物理更新不完整")
        print(f"      - 碰撞检测失效")
    
    print("=" * 60)

if __name__ == "__main__":
    test_ai_behavior()
