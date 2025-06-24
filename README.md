# AI Devour Evolve

一个基于强化学习的细胞吞噬游戏，使用Qt6 + LibTorch开发。

## 项目结构

```
ai-devour-evolve/
├── src/                    # Qt源代码
│   ├── main.cpp           # 主程序入口
│   ├── DemoQtVS.cpp       # 主窗口实现
│   ├── DemoQtVS.h         # 主窗口头文件
│   └── DemoQtVS.ui        # UI界面文件
├── python/                # Python AI训练代码
├── models/                # 训练好的AI模型文件
├── build/                 # 编译输出目录
├── CMakeLists.txt         # CMake配置文件
├── test_torch.cpp         # LibTorch测试文件
└── README.md              # 项目说明
```

## 环境要求

- Qt 6.9.1 (MSVC2022_64)
- LibTorch CPU版本
- Visual Studio 2022
- CMake 3.5+

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
