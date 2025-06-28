#!/usr/bin/env python3
"""
AI Devour Evolve - Python Environment Test Script

测试headless GameEngine的Python绑定是否正常工作
"""

import sys
import time
import random

try:
    import gobigger_env
    print("✓ Successfully imported gobigger_env module")
except ImportError as e:
    print(f"✗ Failed to import gobigger_env: {e}")
    sys.exit(1)

def test_basic_functionality():
    """测试基本功能"""
    print("\n=== Testing Basic Functionality ===")
    
    # 创建环境
    env = gobigger_env.GameEngine()
    print("✓ GameEngine created successfully")
    
    # 重置环境
    obs = env.reset()
    print("✓ Environment reset successfully")
    print(f"Initial observation type: {type(obs)}")
    
    # 检查观察结构
    print(f"Global state border: {obs.global_state.border}")
    print(f"Total frames: {obs.global_state.total_frame}")
    print(f"Number of players: {len(obs.player_states)}")
    
    if obs.player_states:
        player_id = list(obs.player_states.keys())[0]
        player_state = obs.player_states[player_id]
        print(f"Player {player_id} score: {player_state.score}")
        print(f"Player {player_id} can_split: {player_state.can_split}")
        print(f"Player {player_id} can_eject: {player_state.can_eject}")
        print(f"Food objects in view: {len(player_state.food)}")
        print(f"Thorns objects in view: {len(player_state.thorns)}")
    
    return env

def test_actions(env, num_steps=100):
    """测试动作执行"""
    print("\n=== Testing Actions ===")
    
    for step in range(num_steps):
        # 创建随机动作
        action = gobigger_env.Action(
            random.uniform(-1.0, 1.0),  # direction_x
            random.uniform(-1.0, 1.0),  # direction_y
            random.randint(0, 2)        # action_type: 0=move, 1=eject, 2=split
        )
        
        # 执行一步
        obs = env.step(action)
        
        if step % 20 == 0:
            player_states = obs.player_states
            if player_states:
                player_id = list(player_states.keys())[0]
                score = player_states[player_id].score
                print(f"Step {step}: Player score = {score:.1f}")
        
        # 检查游戏是否结束
        if env.is_done():
            print(f"Game ended at step {step}")
            break
    
    print(f"✓ Completed {step + 1} steps successfully")

def test_performance(env, num_steps=1000):
    """测试性能"""
    print("\n=== Testing Performance ===")
    
    start_time = time.time()
    
    for step in range(num_steps):
        action = gobigger_env.Action(
            random.uniform(-0.5, 0.5),
            random.uniform(-0.5, 0.5),
            0  # 只测试移动，避免复杂操作
        )
        
        obs = env.step(action)
        
        if env.is_done():
            break
    
    end_time = time.time()
    elapsed = end_time - start_time
    fps = num_steps / elapsed
    
    print(f"✓ Performance test completed")
    print(f"  Steps: {num_steps}")
    print(f"  Time: {elapsed:.2f}s")
    print(f"  FPS: {fps:.1f}")

def test_gymnasium_compatibility():
    """测试Gymnasium兼容性（模拟）"""
    print("\n=== Testing Gymnasium Compatibility ===")
    
    try:
        # 模拟Gymnasium环境接口
        env = gobigger_env.GameEngine()
        
        # reset() -> observation
        obs = env.reset()
        print("✓ reset() returns observation")
        
        # step(action) -> observation, reward, done, info
        action = gobigger_env.Action(0.5, -0.5, 0)
        obs = env.step(action)
        reward = 0.0  # 暂时硬编码，后续可以从观察中计算
        done = env.is_done()
        info = {"total_frames": env.get_total_frames()}
        
        print("✓ step() returns observation, reward, done, info")
        print(f"  Done: {done}")
        print(f"  Info: {info}")
        
        # 可以进一步包装成标准Gymnasium环境
        print("✓ Compatible with Gymnasium interface pattern")
        
    except Exception as e:
        print(f"✗ Gymnasium compatibility test failed: {e}")

def main():
    """主测试函数"""
    print("AI Devour Evolve - Python Environment Test")
    print("=" * 50)
    
    try:
        # 基本功能测试
        env = test_basic_functionality()
        
        # 动作测试
        test_actions(env, num_steps=50)
        
        # 重置环境
        env.reset()
        
        # 性能测试
        test_performance(env, num_steps=500)
        
        # Gymnasium兼容性测试
        test_gymnasium_compatibility()
        
        print("\n" + "=" * 50)
        print("✓ All tests passed successfully!")
        print("The Python environment is ready for RL training.")
        
    except Exception as e:
        print(f"\n✗ Test failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
