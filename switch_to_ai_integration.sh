#!/bin/bash
# AI集成分支切换脚本

echo "🚀 AI集成分支切换脚本"
echo "=============================="

# 1. 检查当前Git状态
if [[ -n $(git status --porcelain) ]]; then
    echo "⚠️ 发现未提交的更改，请先提交："
    git status --short
    echo ""
    echo "建议运行："
    echo "git add ."
    echo "git commit -m \"🔥 RL训练阶段完成 - Gemini优化+模型导出\""
    exit 1
fi

# 2. 创建导出目录备份
echo "📦 备份AI模型文件..."
mkdir -p ../assets/ai_models
cp scripts/exported_models/ai_model.onnx ../assets/ai_models/ 2>/dev/null || true
cp scripts/exported_models/model_info.json ../assets/ai_models/ 2>/dev/null || true
cp scripts/exported_models/AIModelInterface.h ../assets/ 2>/dev/null || true

# 3. 切换到main分支
echo "🔄 切换到main分支..."
git checkout main

# 4. 创建AI集成分支
echo "🌟 创建AI集成分支..."
git checkout -b ai-integration-dev

# 5. 创建新的文件结构
echo "📁 创建AI集成文件结构..."
mkdir -p src/ai
mkdir -p src/multiplayer  
mkdir -p src/debug
mkdir -p src/ui
mkdir -p assets/ai_models

# 6. 拷贝AI模型文件
echo "📋 拷贝AI模型到新分支..."
cp ../assets/ai_models/ai_model.onnx assets/ai_models/ 2>/dev/null || echo "⚠️ 未找到ONNX模型文件"
cp ../assets/ai_models/model_info.json assets/ai_models/ 2>/dev/null || echo "⚠️ 未找到模型信息文件"
cp ../assets/AIModelInterface.h src/ai/ 2>/dev/null || echo "⚠️ 未找到AI接口头文件"

# 7. 创建初始README
cat > README_AI_INTEGRATION.md << 'EOF'
# AI集成开发分支

## 🎯 目标
将训练好的强化学习AI模型集成到C++游戏引擎中，实现多玩家AI对战系统。

## 📋 任务列表
详见: `need_docs/AI_INTEGRATION_BRANCH_TASKS.md`

## 🚀 快速开始
1. 安装ONNX Runtime C++库
2. 实现AIPlayer类
3. 集成多玩家系统

## 📊 当前状态
- [x] AI模型导出完成
- [ ] ONNX Runtime集成
- [ ] AIPlayer实现
- [ ] 多玩家功能
- [ ] 调试工具

## 🔧 依赖
- ONNX Runtime
- 现有的GoBigger C++引擎
EOF

# 8. 提交初始状态
echo "💾 提交初始AI集成分支状态..."
git add .
git commit -m "🎯 创建AI集成分支 - 初始化项目结构

✅ 完成内容:
- 创建AI集成文件结构 (src/ai, src/multiplayer等)
- 导入训练好的AI模型 (ONNX格式)
- 添加C++集成接口定义
- 项目文档和任务规划

🎯 下一步: 集成ONNX Runtime并实现AIPlayer类"

echo ""
echo "✅ AI集成分支创建成功！"
echo "📍 当前分支: $(git branch --show-current)"
echo ""
echo "📁 项目结构:"
echo "├── src/"
echo "│   ├── ai/              # AI相关代码"
echo "│   ├── multiplayer/     # 多玩家系统" 
echo "│   ├── debug/           # 调试工具"
echo "│   └── ui/              # 用户界面"
echo "├── assets/"
echo "│   └── ai_models/       # AI模型文件"
echo "└── need_docs/           # 开发文档"
echo ""
echo "🔄 下一步操作:"
echo "1. 查看任务列表: cat need_docs/AI_INTEGRATION_BRANCH_TASKS.md"
echo "2. 开始AI集成开发"
echo "3. 实现多玩家功能"
echo ""
echo "🎉 开始AI集成之旅吧！"
