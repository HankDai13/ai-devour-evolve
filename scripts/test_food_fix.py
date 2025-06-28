#!/usr/bin/env python3
"""
简单测试食物分数修复
"""
import sys
import os
from pathlib import Path

# 路径设置
root_dir = Path(__file__).parent.parent
build_dir = root_dir / "build" / "Release"
sys.path.insert(0, str(build_dir))
os.environ["PATH"] = f"{str(build_dir)};{os.environ['PATH']}"

import gobigger_env

print("Testing food score fix...")

engine = gobigger_env.GameEngine()
obs = engine.reset()

# 朝食物移动
action = gobigger_env.Action(1.0, 0.0, 0)
prev_score = obs.player_states[0].score

print(f"Initial score: {prev_score}")

for i in range(30):
    obs = engine.step(action)
    current_score = obs.player_states[0].score
    score_change = current_score - prev_score
    
    if score_change > 0:
        print(f"Frame {i+1}: Score changed from {prev_score} to {current_score} (+{score_change})")
        print("SUCCESS: Food eating works!")
        break
    
    prev_score = current_score

if obs.player_states[0].score == 1000.0:
    print("FAILED: Score still unchanged")
else:
    print(f"Final score: {obs.player_states[0].score}")
