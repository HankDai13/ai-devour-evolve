# AI Devour Evolve - 项目结构

## 📁 主要目录结构

```
ai-devour-evolve/
├── 📄 CMakeLists.txt              # 主构建配置文件
├── 📄 README.md                   # 项目说明文档
├── 📄 LICENSE                     # 项目许可证
├── 📄 .gitignore                  # Git忽略文件配置
├── 🔧 build_and_run.bat          # 构建和运行脚本
├── 🔧 run.bat                     # 快速运行脚本
│
├── 📁 .vscode/                    # VS Code配置文件
│   ├── tasks.json                 # 构建任务配置
│   ├── launch.json                # 调试配置
│   └── settings.json              # 编辑器设置
│
├── 📁 src/                        # 🎯 核心源代码目录
│   ├── 🎮 main.cpp                # 主程序入口
│   ├── 🎮 GameManager.cpp/h       # 游戏管理器
│   ├── 🎮 GameView.cpp/h          # 游戏视图
│   ├── 🤖 SimpleAIPlayer.cpp/h    # AI玩家实现
│   ├── ⚽ BaseBall.cpp/h          # 基础球体类
│   ├── ⚽ CloneBall.cpp/h         # 玩家球体类
│   ├── 🍎 FoodBall.cpp/h          # 食物球体类
│   ├── 💫 SporeBall.cpp/h         # 孢子球体类
│   ├── 🌵 ThornsBall.cpp/h        # 荆棘球体类
│   ├── 🧠 ONNXInference.cpp/h     # ONNX模型推理
│   ├── 🏗️ QuadTree.cpp/h          # 四叉树空间划分
│   └── 📁 core/                   # 核心引擎组件
│       ├── GameEngine.cpp/h       # 游戏引擎
│       └── data/                  # 数据结构
│
├── 📁 assets/                     # 📦 资源文件目录
│   └── ai_models/                 # AI模型文件
│       ├── simple_gobigger_demo.onnx
│       └── exported_models/       # 导出的AI模型
│
├── 📁 python/                     # 🐍 Python绑定和环境
│   ├── bindings.cpp               # Python C++绑定
│   ├── gobigger_env.pyd          # 编译的Python模块
│   └── multi_agent_*.py          # 多智能体环境
│
├── 📁 scripts/                    # 📊 训练和测试脚本
│   ├── train_rl_agent.py         # 强化学习训练
│   ├── evaluate_model.py         # 模型评估
│   ├── debug_*.py                # 调试脚本
│   ├── checkpoints/               # 训练检查点
│   └── models/                    # 训练保存的模型
│
├── 📁 develop-Documents/          # 📚 开发文档
│   ├── phase1.md                  # 开发阶段1文档
│   ├── environment-setup-guide.md # 环境搭建指南
│   └── report/                    # 项目报告
│
├── 📁 need_docs/                  # 📝 需求和待办文档
│   ├── 0708TODO.md               # 最新待办事项
│   ├── AI_INTEGRATION_BRANCH_TASKS.md
│   └── finished.md               # 已完成功能
│
├── 📁 models/                     # 🤖 AI模型存储目录
└── 📁 build/                      # 🏗️ 构建输出目录 (自动生成)
    ├── ai-devour-evolve.exe      # 主程序可执行文件
    └── Release/                   # 发布版本文件
```

## 🎯 核心功能模块

### 🎮 游戏核心 (`src/`)
- **GameManager**: 游戏逻辑管理、碰撞检测、物理模拟
- **GameView**: Qt图形界面、用户交互、渲染
- **BaseBall 系列**: 游戏中各种球体的实现

### 🤖 AI系统 (`src/SimpleAIPlayer.*`)
- **启发式AI**: 食物猎手、攻击型、随机等策略
- **神经网络AI**: 基于ONNX模型的深度学习AI
- **多智能体**: 支持多个AI同时对战

### 🐍 Python环境 (`python/`, `scripts/`)
- **强化学习**: 基于Stable-Baselines3的训练环境
- **模型训练**: PPO、A2C等算法支持
- **性能评估**: 自动化测试和评估脚本

### 📦 构建系统
- **CMake**: 跨平台构建配置
- **Qt6**: 图形界面框架
- **ONNX Runtime**: 神经网络推理引擎

## 🚀 快速开始

1. **构建项目**:
   ```bash
   # Windows
   ./build_and_run.bat
   
   # 或使用VS Code任务
   Ctrl+Shift+P -> Tasks: Run Task -> Build Main Application
   ```

2. **运行游戏**:
   ```bash
   ./run.bat
   ```

3. **训练AI**:
   ```bash
   cd scripts
   python train_rl_agent.py
   ```

## 📚 更多文档

- 🔧 **环境搭建**: `develop-Documents/environment-setup-guide.md`
- 📖 **开发指南**: `develop-Documents/phase1.md`
- ✅ **功能状态**: `need_docs/finished.md`
- 📋 **待办事项**: `need_docs/0708TODO.md`
