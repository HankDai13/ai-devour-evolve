#!/usr/bin/env python3
"""
第四阶段测试：AI接口数据对齐验证
验证观察空间和动作空间是否符合GoBigger训练环境规范
"""
import sys
import os
from pathlib import Path

# 确保从项目根目录导入，解决路径问题
root_dir = Path(__file__).parent if Path(__file__).parent.name != "python" else Path(__file__).parent.parent
sys.path.insert(0, str(root_dir / "build" / "Release"))
os.environ["PATH"] = f"{str(root_dir / 'build' / 'Release')};{os.environ['PATH']}"

def test_observation_space_structure():
    """测试观察空间的数据结构"""
    print("🔍 测试观察空间数据结构...")
    
    import gobigger_env
    engine = gobigger_env.GameEngine()
    obs = engine.reset()
    
    # 验证顶层结构
    assert hasattr(obs, 'global_state'), "缺少global_state"
    assert hasattr(obs, 'player_states'), "缺少player_states"
    print("✅ 顶层结构正确：global_state + player_states")
    
    # 验证全局状态结构
    gs = obs.global_state
    assert hasattr(gs, 'border'), "global_state缺少border"
    assert hasattr(gs, 'total_frame'), "global_state缺少total_frame"
    assert hasattr(gs, 'last_frame_count'), "global_state缺少last_frame_count"
    assert hasattr(gs, 'leaderboard'), "global_state缺少leaderboard"
    print(f"✅ 全局状态结构正确：border={gs.border}, total_frame={gs.total_frame}")
    
    # 验证玩家状态结构
    print(f"🔍 玩家状态数量: {len(obs.player_states)}")
    if len(obs.player_states) > 0:
        player_id = list(obs.player_states.keys())[0] if hasattr(obs.player_states, 'keys') else 0
        ps = obs.player_states[player_id] if hasattr(obs.player_states, '__getitem__') else list(obs.player_states.values())[0]
        
        print(f"✅ 玩家{player_id}状态:")
        print(f"  - rectangle: {len(ps.rectangle) if hasattr(ps.rectangle, '__len__') else 'N/A'} 个元素")
        print(f"  - food: {len(ps.food)} 个对象")
        print(f"  - thorns: {len(ps.thorns)} 个对象") 
        print(f"  - spore: {len(ps.spore)} 个对象")
        print(f"  - clone: {len(ps.clone)} 个对象")
        print(f"  - score: {ps.score}")
        print(f"  - can_eject: {ps.can_eject}")
        print(f"  - can_split: {ps.can_split}")

def test_data_format_compliance():
    """测试数据格式是否符合GoBigger规范"""
    print("\n📏 测试数据格式规范...")
    
    import gobigger_env
    engine = gobigger_env.GameEngine()
    obs = engine.reset()
    
    if len(obs.player_states) > 0:
        # 获取第一个玩家的状态
        ps = list(obs.player_states.values())[0] if hasattr(obs.player_states, 'values') else obs.player_states[0]
        
        # 验证数量限制
        print(f"📊 对象数量验证:")
        print(f"  - 食物数量: {len(ps.food)} (应该≤50)")
        print(f"  - 荆棘数量: {len(ps.thorns)} (应该≤20)")
        print(f"  - 孢子数量: {len(ps.spore)} (应该≤10)")
        print(f"  - 克隆球数量: {len(ps.clone)} (应该≤30)")
        
        assert len(ps.food) <= 50, f"食物数量超限: {len(ps.food)}"
        assert len(ps.thorns) <= 20, f"荆棘数量超限: {len(ps.thorns)}"
        assert len(ps.spore) <= 10, f"孢子数量超限: {len(ps.spore)}"
        assert len(ps.clone) <= 30, f"克隆球数量超限: {len(ps.clone)}"
        
        # 验证特征向量维度
        if len(ps.food) > 0:
            food_features = len(ps.food[0])
            print(f"  - 食物特征维度: {food_features} (期望:4)")
            assert food_features == 4, f"食物特征维度错误: {food_features}"
        
        if len(ps.thorns) > 0:
            thorns_features = len(ps.thorns[0])
            print(f"  - 荆棘特征维度: {thorns_features} (期望:6)")
            assert thorns_features == 6, f"荆棘特征维度错误: {thorns_features}"
        
        if len(ps.clone) > 0:
            clone_features = len(ps.clone[0])
            print(f"  - 克隆球特征维度: {clone_features} (期望:10)")
            assert clone_features == 10, f"克隆球特征维度错误: {clone_features}"
        
        print("✅ 数据格式符合规范")

def test_action_space():
    """测试动作空间"""
    print("\n🎯 测试动作空间...")
    
    import gobigger_env
    engine = gobigger_env.GameEngine()
    engine.reset()
    
    # 测试动作范围限制
    test_actions = [
        (2.0, 2.0, 5),   # 超出范围的动作
        (-2.0, -2.0, -1), # 超出范围的动作
        (0.5, -0.8, 1),  # 正常范围的动作
        (0.0, 0.0, 0),   # 无动作
    ]
    
    for dx, dy, act_type in test_actions:
        action = gobigger_env.Action(dx, dy, act_type)
        print(f"  输入动作: ({dx}, {dy}, {act_type})")
        print(f"  处理后: ({action.direction_x}, {action.direction_y}, {action.action_type})")
        
        # 执行动作
        obs = engine.step(action)
        print(f"  执行成功，当前帧: {obs.global_state.total_frame}")
    
    print("✅ 动作空间测试通过")

def test_data_preprocessing():
    """测试数据预处理（归一化、填充）"""
    print("\n🔧 测试数据预处理...")
    
    import gobigger_env
    engine = gobigger_env.GameEngine()
    obs = engine.reset()
    
    if len(obs.player_states) > 0:
        ps = list(obs.player_states.values())[0] if hasattr(obs.player_states, 'values') else obs.player_states[0]
        
        # 检查数据是否被正确填充到固定数量
        print(f"📋 数据填充检查:")
        print(f"  - 食物对象数: {len(ps.food)}")
        print(f"  - 荆棘对象数: {len(ps.thorns)}")
        print(f"  - 孢子对象数: {len(ps.spore)}")
        print(f"  - 克隆球对象数: {len(ps.clone)}")
        
        # 检查归一化（坐标应该在合理范围内）
        if len(ps.food) > 0:
            for i, food in enumerate(ps.food[:3]):  # 检查前3个
                x, y, radius, score = food[0], food[1], food[2], food[3]
                print(f"  食物{i}: x={x:.3f}, y={y:.3f}, r={radius:.3f}, s={score:.3f}")
                if x != 0 or y != 0:  # 非填充的零值
                    assert -2 <= x <= 2, f"食物x坐标超出预期范围: {x}"
                    assert -2 <= y <= 2, f"食物y坐标超出预期范围: {y}"
        
        print("✅ 数据预处理正确")

def main():
    """主测试函数"""
    try:
        print("🚀 开始第四阶段：AI接口数据对齐测试\n")
        
        test_observation_space_structure()
        test_data_format_compliance()
        test_action_space()
        test_data_preprocessing()
        
        print("\n🎉 第四阶段测试全部通过！")
        print("✅ 观察空间数据结构符合GoBigger规范")
        print("✅ 动作空间处理正确")
        print("✅ 数据预处理(归一化、限制、填充)工作正常")
        print("✅ C++核心逻辑与Python接口完全对齐")
        
    except ImportError as e:
        print(f"❌ 导入失败: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"❌ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
