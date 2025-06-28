#!/usr/bin/env python3
"""
调试脚本：检查 GoBigger 环境的观察数据结构
"""

import gobigger
from gobigger.envs import create_env
import numpy as np
import json

def debug_observation_structure():
    """调试观察数据的结构"""
    
    # 创建环境
    env_config = {
        'team_num': 4,
        'player_num_per_team': 1, 
        'match_time': 300,
        'map_height': 1000,
        'map_width': 1000,
        'vision_num': 1,
        'max_step_in_game': 10000
    }
    
    env = create_env('st_t4p1', cfg=env_config)
    print("环境创建成功")
    
    # 重置环境获取初始观察
    obs = env.reset()
    print(f"观察数据键: {list(obs.keys())}")
    
    # 检查每个玩家的观察数据
    for player_id, player_obs in obs.items():
        print(f"\n玩家 {player_id} 的观察数据结构:")
        print(f"  键: {list(player_obs.keys())}")
        
        # 检查克隆球数据
        if 'clone' in player_obs:
            clone_data = player_obs['clone']
            print(f"  克隆球数据类型: {type(clone_data)}")
            print(f"  克隆球数据长度: {len(clone_data) if hasattr(clone_data, '__len__') else 'N/A'}")
            
            if isinstance(clone_data, list) and len(clone_data) > 0:
                print(f"  第一个克隆球类型: {type(clone_data[0])}")
                print(f"  第一个克隆球内容: {clone_data[0]}")
                
                # 如果是字典，显示键
                if isinstance(clone_data[0], dict):
                    print(f"  第一个克隆球的键: {list(clone_data[0].keys())}")
                elif isinstance(clone_data[0], list):
                    print(f"  第一个克隆球列表长度: {len(clone_data[0])}")
                    print(f"  第一个克隆球列表内容: {clone_data[0]}")
        
        # 检查食物球数据
        if 'food' in player_obs:
            food_data = player_obs['food']
            print(f"  食物球数据类型: {type(food_data)}")
            print(f"  食物球数据长度: {len(food_data) if hasattr(food_data, '__len__') else 'N/A'}")
            
            if isinstance(food_data, list) and len(food_data) > 0:
                print(f"  第一个食物球类型: {type(food_data[0])}")
                if isinstance(food_data[0], list):
                    print(f"  第一个食物球列表长度: {len(food_data[0])}")
                    print(f"  第一个食物球列表内容: {food_data[0]}")
        
        # 检查荆棘球数据
        if 'thorns' in player_obs:
            thorns_data = player_obs['thorns']
            print(f"  荆棘球数据类型: {type(thorns_data)}")
            print(f"  荆棘球数据长度: {len(thorns_data) if hasattr(thorns_data, '__len__') else 'N/A'}")
            
            if isinstance(thorns_data, list) and len(thorns_data) > 0:
                print(f"  第一个荆棘球类型: {type(thorns_data[0])}")
                if isinstance(thorns_data[0], list):
                    print(f"  第一个荆棘球列表长度: {len(thorns_data[0])}")
                    print(f"  第一个荆棘球列表内容: {thorns_data[0]}")
        
        # 只检查第一个玩家就够了
        break
    
    # 执行几步看看数据变化
    print("\n执行一个动作后的观察数据:")
    actions = {player_id: [0, 0, 0] for player_id in obs.keys()}
    obs, rewards, dones, infos = env.step(actions)
    
    for player_id, player_obs in obs.items():
        print(f"\n玩家 {player_id} 动作后的观察数据:")
        
        # 再次检查克隆球数据
        if 'clone' in player_obs:
            clone_data = player_obs['clone']
            print(f"  克隆球数据长度: {len(clone_data) if hasattr(clone_data, '__len__') else 'N/A'}")
            
            if isinstance(clone_data, list) and len(clone_data) > 0:
                for i, ball in enumerate(clone_data[:3]):  # 只看前3个
                    print(f"  克隆球 {i}: 类型={type(ball)}, 内容={ball}")
        
        break
    
    env.close()

if __name__ == "__main__":
    debug_observation_structure()
