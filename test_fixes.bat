@echo off
echo 测试AI修复效果...
echo.
echo 请按照以下步骤测试：
echo 1. 启动游戏后，按P键开始游戏
echo 2. 打开菜单 AI -> 显示AI调试控制台 (F12)
echo 3. 测试快速添加AI玩家 - 检查COLOR是否正确显示
echo 4. 测试快速添加RL-AI玩家 - 检查是否为攻击性策略而非食物猎手
echo 5. 测试自定义添加AI玩家，选择模型驱动类型 - 检查球是否可见
echo 6. 分裂测试：让球长大后分裂多次，观察分裂数量限制(10个)和向心力是否平滑
echo.
echo 按任意键启动游戏...
pause >nul

cd /d d:\Coding\Projects\ai-devour-evolve
.\run.bat
