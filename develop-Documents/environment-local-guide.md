# 本地开发环境配置指南

## 📋 环境概览

本文档记录了AI Devour Evolve项目在Windows环境下的完整配置信息，供AI助手参考使用。

---

## 🛠 开发环境信息

### 操作系统
- **系统**: Windows 10/11
- **架构**: x64
- **默认Shell**: PowerShell (`powershell.exe`)

### 编译器
- **编译器**: MSVC 19.40.33811.0 (Visual Studio 2022 BuildTools)
- **标准**: C++17
- **架构**: x64

---

## 📂 关键路径配置

### Qt 安装路径
```
主路径: D:/qt/
版本: 6.9.1
MSVC版本: D:/qt/6.9.1/msvc2022_64
MinGW版本: D:/qt/6.9.1/mingw_64
```

### Qt 工具路径
```
CMake: D:/qt/Tools/CMake_64/bin/cmake.exe
MinGW: D:/qt/Tools/mingw1310_64/bin
windeployqt (MSVC): D:/qt/6.9.1/msvc2022_64/bin/windeployqt.exe
```

### LibTorch 路径
```
LibTorch: D:/tools/libtorch-cpu/libtorch
```

### 项目路径
```
工作目录: d:\Coding\Projects\ai-devour-evolve
构建目录: d:\Coding\Projects\ai-devour-evolve\build
输出目录: d:\Coding\Projects\ai-devour-evolve\build\Release\
```

---

## ⚙️ CMake 配置

### 推荐的CMake配置命令
```bash
# 配置项目 (使用MSVC - 推荐)
D:/qt/Tools/CMake_64/bin/cmake.exe -B build -S . -G "Visual Studio 17 2022" -A x64

# 编译项目
D:/qt/Tools/CMake_64/bin/cmake.exe --build build --config Release
```

### CMakeLists.txt 关键配置
```cmake
# Qt和LibTorch路径设置
if(MSVC)
    set(CMAKE_PREFIX_PATH "D:/qt/6.9.1/msvc2022_64;D:/tools/libtorch-cpu/libtorch")
elseif(MINGW)
    set(CMAKE_PREFIX_PATH "D:/qt/6.9.1/mingw_64;D:/tools/libtorch-cpu/libtorch")
endif()

# C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt组件
find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(Torch REQUIRED)
```

---

## 🔧 编译流程

### 完整编译流程
```bash
# 1. 清理构建目录
Remove-Item -Recurse -Force build
mkdir build

# 2. 配置CMake
D:/qt/Tools/CMake_64/bin/cmake.exe -B build -S . -G "Visual Studio 17 2022" -A x64

# 3. 编译项目
D:/qt/Tools/CMake_64/bin/cmake.exe --build build --config Release

# 4. 部署Qt DLL
D:/qt/6.9.1/msvc2022_64/bin/windeployqt.exe --release build/Release/ai-devour-evolve.exe

# 5. 运行程序
./build/Release/ai-devour-evolve.exe
```

### 自动化脚本
项目根目录提供了两个批处理脚本：
- `build_and_run.bat` - 完整的构建和运行
- `run_only.bat` - 快速运行已编译程序

---

## 🚨 常见问题及解决方案

### 1. 链接错误 (LNK2019)
**原因**: Qt库未正确链接或编译器不匹配
**解决**: 
- 确保使用MSVC编译器而非MinGW
- 检查Qt路径是否正确
- 重新配置CMake

### 2. 程序无法启动 (缺少DLL)
**原因**: Qt运行时库未部署
**解决**:
```bash
D:/qt/6.9.1/msvc2022_64/bin/windeployqt.exe --release build/Release/ai-devour-evolve.exe
```

### 3. CMake配置失败
**原因**: 路径错误或编译器未找到
**解决**:
- 确认Visual Studio 2022 BuildTools已安装
- 检查Qt和LibTorch路径是否存在
- 清理build目录重新配置

---

## 📦 依赖库信息

### Qt 6.9.1 组件
- `Qt6::Core` - 核心功能
- `Qt6::Widgets` - GUI组件
- `Qt6::Gui` - 图形界面

### LibTorch
- **版本**: 2.7.1
- **类型**: CPU版本 (禁用CUDA)
- **路径**: D:/tools/libtorch-cpu/libtorch

---

## 🎯 项目结构

```
ai-devour-evolve/
├── src/
│   ├── main.cpp              # 主程序入口
│   ├── GameView.h/.cpp       # 游戏视图
│   ├── PlayerCell.h/.cpp     # 玩家细胞
│   ├── FoodItem.h/.cpp       # 食物系统
│   └── DemoQtVS.h/.cpp/.ui   # Demo组件
├── build/                    # 构建目录
│   └── Release/
│       └── ai-devour-evolve.exe
├── develop-Documents/        # 开发文档
├── CMakeLists.txt           # 构建配置
├── build_and_run.bat       # 自动构建脚本
└── run_only.bat            # 快速运行脚本
```

---

## 🔄 Git 分支信息

- **当前分支**: gobigger-redo
- **状态**: 工作正常，可编译运行
- **最后备份**: 2025年6月26日

---

## 💡 AI助手使用建议

1. **优先使用MSVC编译器**，避免MinGW相关问题
2. **始终检查路径存在性**，特别是Qt和LibTorch路径
3. **编译前清理build目录**，避免缓存问题
4. **使用windeployqt部署DLL**，确保程序可运行
5. **遇到问题时参考本文档的路径配置**

---

## ✅ 验证环境正常的步骤

```bash
# 1. 检查Qt路径
ls D:/qt/6.9.1/msvc2022_64/bin/

# 2. 检查CMake
D:/qt/Tools/CMake_64/bin/cmake.exe --version

# 3. 测试编译
cd d:\Coding\Projects\ai-devour-evolve
./build_and_run.bat
```

---

**最后更新**: 2025年6月26日  
**验证状态**: ✅ 环境正常，可编译运行
