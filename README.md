# AI Devour Evolve

一个基于强化学习的细胞吞噬游戏，使用Qt6 + LibTorch开发。

## 🎉 Phase 1 完成！ (完整原型实现)

**完整游戏原型已实现！** 包含：
- 响应键盘操作的玩家细胞 (WASD + 方向键)
- 完整的Qt6图形渲染框架
- 流畅的游戏交互体验 
- 60FPS的平滑移动系统
- 完美的视觉渲染，无残影无卡顿
- **🍎 食物生成系统**: 随机生成彩色食物点
- **💥 碰撞检测**: 实时碰撞检测和吞噬机制
- **📈 成长系统**: 玩家吞噬食物后持续成长
- **🏆 得分系统**: 实时计算和显示玩家得分

### 快速开始
```bash
# 1. Clone项目
git clone [你的仓库地址]
cd ai-devour-evolve

# 2. 用VS Code打开项目
code .

# 3. 按照环境配置指南设置开发环境
# 详见：develop-Documents/environment-setup-guide.md

# 4. 一键编译运行
# 按F7编译，按F5运行，立即体验游戏原型！
# 使用WASD或方向键控制蓝色细胞移动
```

### 🎮 游戏操作
- **W键/↑键**: 向上移动
- **S键/↓键**: 向下移动
- **A键/←键**: 向左移动  
- **D键/→键**: 向右移动
- **组合按键**: 支持斜向移动 (如 W+D = 右上)

### 🎯 游戏目标
- 控制蓝色细胞在场景中移动
- 吞噬随机生成的彩色食物点
- 通过吞噬食物让细胞不断成长
- 获得更高的得分！

## 项目结构

```
ai-devour-evolve/
├── src/                    # Qt源代码
│   ├── main.cpp           # 主程序入口  
│   ├── GameView.h/cpp     # 游戏视图类 (新增)
│   ├── PlayerCell.h/cpp   # 玩家细胞类 (新增)
│   ├── DemoQtVS.cpp       # 主窗口实现
│   ├── DemoQtVS.h         # 主窗口头文件
│   └── DemoQtVS.ui        # UI界面文件
├── develop-Documents/     # 开发文档 (新增)
│   ├── phase1.md         # Phase 1开发指南
│   ├── phase1-completion-report.md  # 完成报告
│   └── environment-setup-guide.md  # 环境配置指南
├── python/                # Python AI训练代码
├── models/                # 训练好的AI模型文件
├── build/                 # 编译输出目录
├── CMakeLists.txt         # CMake配置文件
├── COLLABORATION.md       # Git协作流程文档 (新增)
├── test_torch.cpp         # LibTorch测试文件
└── README.md              # 项目说明
```

## 环境要求

- Qt 6.9.1 (MSVC2022_64 或 MinGW)
- LibTorch CPU版本
- Visual Studio 2022 或 MinGW
- CMake 3.5+
- VS Code + C++ + CMake Tools扩展

## 编译指南

```bash
# 配置项目
cmake -B build -S . -G "Visual Studio 17 2022" -A x64

# 编译项目
cmake --build build --config Release
```

## 团队分工

- **队员A (HankDai13)**: Python AI训练总负责人 + C++项目架构师与AI集成工程师
- **队员B**: C++网络与实现工程师

## 协作流程

请参考项目根目录下的 `COLLABORATION.md` 文件。
