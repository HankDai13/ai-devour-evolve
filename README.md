# AI Devour Evolve

> 🚀 C++/Qt高性能复刻GoBigger单人玩法，所有机制、数值、体验全面对齐原版，支持AI集成与二次开发。

## 项目简介

AI Devour Evolve 致力于**完整复现GoBigger单人玩法**，采用C++/Qt开发，核心机制、物理、数值、动作空间、视觉体验等全部对齐GoBigger原版。支持高密度食物、荆棘球、孢子球、分裂、合并等全部玩法，性能极致优化，体验流畅不卡顿。

---

## 主要特性

- 🟦 **玩家球/分身球**：支持分裂、合并、吞噬、冷却、刚体推开等全部机制
- 🍎 **食物球**：高密度生成，性能优化，吞噬成长完全对齐GoBigger
- 🟢 **孢子球**：喷射、碰撞、被荆棘球吸收等机制完整实现
- 🌵 **荆棘球**：生成、分裂、分数范围、冷却、孢子加速等全部机制
- 🧠 **动作空间/物理机制**：所有参数、判定、冷却、分数体系与GoBigger一致
- 🔍 **四叉树空间分区**：支持3000+食物/荆棘球不卡顿，极致性能
- 🎮 **视角缩放/分数显示**：动态缩放、总分数UI、体验与原版一致
- 🛠️ **代码结构清晰**：便于AI接入、二次开发、机制扩展

---

## 编译与运行

### 环境要求
- Windows 10/11 + Visual Studio 2022 + Qt 6.5+ (建议6.9.1)
- CMake 3.20+
- LibTorch CPU版 (可选AI集成)
- VS Code + C++/CMake Tools扩展

### 一键编译运行
```bash
# 1. 克隆项目
 git clone [你的仓库地址]
 cd ai-devour-evolve

# 2. 配置CMake (MSVC)
 cmake -B build -S . -G "Visual Studio 17 2022" -A x64

# 3. 编译主程序
 cmake --build build --config Release

# 4. 运行主程序
 ./build/bin/Release/ai-devour-evolve.exe
```
或直接用VS Code任务面板：
- `Configure CMake (MSVC)` → `Build Main Application` → `Run Application`

---

## 玩法与机制对齐说明

- **分裂/合并/冷却**：所有分裂球冷却期、合并判定、推开/吸附、分裂速度等全部与GoBigger一致
- **荆棘球机制**：生成、分裂、分数分配、孢子加速、冷却等全部参数与原版同步
- **食物/孢子/分身**：吞噬、喷射、碰撞、成长、分数体系完全一致
- **性能优化**：四叉树空间分区，支持高密度食物/荆棘球不卡顿，体验接近GoBigger原版
- **视角缩放/分数显示**：动态缩放、总分数UI、体验与原版一致

---

## 目录结构
```
ai-devour-evolve/
├── src/                # C++/Qt核心代码（全部机制）
├── models/             # AI模型（可选）
├── python/             # AI训练/测试脚本
├── build/              # 编译输出
├── develop-Documents/  # 开发文档/环境配置
├── CMakeLists.txt      # CMake配置
└── README.md           # 项目说明
```

---

## 未来计划 / TODO
- 持续体验和细节优化，确保所有机制与GoBigger原版完全一致
- 增强AI接口与训练支持，开放更多自定义玩法
- 欢迎PR、建议与二次开发，共同完善高性能吞噬AI平台

---

## 致谢
- GoBigger开源项目及其社区
- 所有贡献者与测试者

---

如需详细机制说明、AI集成、二次开发等请查阅 `develop-Documents/` 或联系作者。
