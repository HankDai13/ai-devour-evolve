@echo off
REM AI集成分支切换脚本 (Windows版本)

echo 🚀 AI集成分支切换脚本
echo ==============================

REM 1. 检查Git状态
git status --porcelain > nul 2>&1
if errorlevel 1 (
    echo ❌ Git仓库状态检查失败
    pause
    exit /b 1
)

for /f %%i in ('git status --porcelain') do (
    echo ⚠️ 发现未提交的更改，请先提交：
    git status --short
    echo.
    echo 建议运行：
    echo git add .
    echo git commit -m "🔥 RL训练阶段完成 - Gemini优化+模型导出"
    pause
    exit /b 1
)

REM 2. 备份AI模型文件
echo 📦 备份AI模型文件...
if not exist "..\assets" mkdir "..\assets"
if not exist "..\assets\ai_models" mkdir "..\assets\ai_models"
copy "scripts\exported_models\ai_model.onnx" "..\assets\ai_models\" >nul 2>&1
copy "scripts\exported_models\model_info.json" "..\assets\ai_models\" >nul 2>&1
copy "scripts\exported_models\AIModelInterface.h" "..\assets\" >nul 2>&1

REM 3. 切换到main分支
echo 🔄 切换到main分支...
git checkout main
if errorlevel 1 (
    echo ❌ 切换到main分支失败
    pause
    exit /b 1
)

REM 4. 创建AI集成分支
echo 🌟 创建AI集成分支...
git checkout -b ai-integration-dev
if errorlevel 1 (
    echo ❌ 创建AI集成分支失败
    pause
    exit /b 1
)

REM 5. 创建文件结构
echo 📁 创建AI集成文件结构...
if not exist "src" mkdir "src"
if not exist "src\ai" mkdir "src\ai"
if not exist "src\multiplayer" mkdir "src\multiplayer"
if not exist "src\debug" mkdir "src\debug"
if not exist "src\ui" mkdir "src\ui"
if not exist "assets" mkdir "assets"
if not exist "assets\ai_models" mkdir "assets\ai_models"

REM 6. 拷贝AI模型文件
echo 📋 拷贝AI模型到新分支...
copy "..\assets\ai_models\ai_model.onnx" "assets\ai_models\" >nul 2>&1 || echo ⚠️ 未找到ONNX模型文件
copy "..\assets\ai_models\model_info.json" "assets\ai_models\" >nul 2>&1 || echo ⚠️ 未找到模型信息文件
copy "..\assets\AIModelInterface.h" "src\ai\" >nul 2>&1 || echo ⚠️ 未找到AI接口头文件

REM 7. 创建初始README
echo # AI集成开发分支 > README_AI_INTEGRATION.md
echo. >> README_AI_INTEGRATION.md
echo ## 🎯 目标 >> README_AI_INTEGRATION.md
echo 将训练好的强化学习AI模型集成到C++游戏引擎中，实现多玩家AI对战系统。 >> README_AI_INTEGRATION.md
echo. >> README_AI_INTEGRATION.md
echo ## 📋 任务列表 >> README_AI_INTEGRATION.md
echo 详见: `need_docs/AI_INTEGRATION_BRANCH_TASKS.md` >> README_AI_INTEGRATION.md
echo. >> README_AI_INTEGRATION.md
echo ## 🚀 快速开始 >> README_AI_INTEGRATION.md
echo 1. 安装ONNX Runtime C++库 >> README_AI_INTEGRATION.md
echo 2. 实现AIPlayer类 >> README_AI_INTEGRATION.md
echo 3. 集成多玩家系统 >> README_AI_INTEGRATION.md

REM 8. 提交初始状态
echo 💾 提交初始AI集成分支状态...
git add .
git commit -m "🎯 创建AI集成分支 - 初始化项目结构

✅ 完成内容:
- 创建AI集成文件结构 (src/ai, src/multiplayer等)
- 导入训练好的AI模型 (ONNX格式)  
- 添加C++集成接口定义
- 项目文档和任务规划

🎯 下一步: 集成ONNX Runtime并实现AIPlayer类"

echo.
echo ✅ AI集成分支创建成功！
for /f %%i in ('git branch --show-current') do set current_branch=%%i
echo 📍 当前分支: %current_branch%
echo.
echo 📁 项目结构:
echo ├── src/
echo │   ├── ai/              # AI相关代码
echo │   ├── multiplayer/     # 多玩家系统
echo │   ├── debug/           # 调试工具
echo │   └── ui/              # 用户界面
echo ├── assets/
echo │   └── ai_models/       # AI模型文件
echo └── need_docs/           # 开发文档
echo.
echo 🔄 下一步操作:
echo 1. 查看任务列表: type need_docs\AI_INTEGRATION_BRANCH_TASKS.md
echo 2. 开始AI集成开发
echo 3. 实现多玩家功能
echo.
echo 🎉 开始AI集成之旅吧！
echo.
pause
