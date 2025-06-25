# AI Devour Evolve - 环境配置指南

本文档详细说明了如何为 AI Devour Evolve 项目配置开发环境。请按照以下步骤进行配置。

## 📋 系统要求

- **操作系统**: Windows 10/11 64位
- **内存**: 至少 8GB RAM（推荐 16GB）
- **硬盘**: 至少 20GB 可用空间
- **网络**: 稳定的互联网连接（用于下载依赖）

## 🛠️ 必需软件清单

### 1. Git
- **版本**: 2.30+ 
- **下载**: https://git-scm.com/download/win
- **验证**: `git --version`

### 2. Visual Studio 2022 (推荐) 或 Visual Studio Build Tools
- **版本**: Community 2022 或更高
- **下载**: https://visualstudio.microsoft.com/zh-hans/downloads/
- **必需组件**:
  - MSVC v143 编译器工具集
  - Windows 10/11 SDK (推荐最新版本)
  - CMake 工具

### 3. Qt 6.9.1
- **下载**: https://www.qt.io/download-qt-installer
- **必需组件**:
  - Qt 6.9.1 - MSVC 2022 64-bit
  - Qt 6.9.1 - MinGW 64-bit (可选，作为备用)
  - Qt Creator (可选)
  - CMake 3.5+

### 4. LibTorch (CPU版本)
- **下载**: https://pytorch.org/get-started/locally/
- **配置**: 选择 LibTorch → C++ → CPU → Download
- **版本**: 最新稳定版 (推荐 2.0+)

### 5. Visual Studio Code (推荐IDE)
- **下载**: https://code.visualstudio.com/
- **必需扩展**:
  - C/C++ Extension Pack (Microsoft)
  - CMake Tools (Microsoft)
  - Qt tools (tonka3000.qtvsctools) (可选)

## 📂 目录结构约定

请按照以下目录结构安装软件：

```
C:/
├── Program Files (x86)/
│   └── Microsoft Visual Studio/
│       └── 2022/
│           ├── Community/          # 或 BuildTools/
│           └── ...
├── Program Files/
│   └── Microsoft Visual Studio/
│       └── 2022/
│           └── Community/          # 64位组件
└── ...

D:/                                 # 建议安装到D盘（如果有）
├── qt/
│   ├── 6.9.1/
│   │   ├── msvc2022_64/           # Qt MSVC版本
│   │   └── mingw_64/              # Qt MinGW版本（可选）
│   └── Tools/
│       ├── CMake_64/
│       └── mingw1310_64/
└── tools/
    └── libtorch-cpu/
        └── libtorch/              # LibTorch CPU版本
            ├── include/
            ├── lib/
            └── share/
```

## ⚙️ 详细安装步骤

### 步骤 1: 安装 Visual Studio 2022

1. 下载 Visual Studio Installer
2. 选择 "Visual Studio Community 2022"
3. 在"工作负载"中选择：
   - **C++ 桌面开发**
4. 在"单个组件"中确保包含：
   - MSVC v143 - VS 2022 C++ x64/x86 生成工具
   - Windows 10/11 SDK
   - CMake 工具
5. 完成安装

### 步骤 2: 安装 Qt 6.9.1

1. 下载并运行 Qt Online Installer
2. 创建 Qt 账户并登录
3. 选择安装路径（建议 `D:\qt`）
4. 在组件选择中：
   - 展开 "Qt 6.9.1"
   - ✅ 勾选 "MSVC 2022 64-bit"
   - ✅ 勾选 "MinGW 64-bit" (可选)
   - ✅ 勾选 "Qt Creator" (可选)
   - ✅ 勾选 "CMake"
5. 完成安装

### 步骤 3: 安装 LibTorch

1. 访问 https://pytorch.org/get-started/locally/
2. 选择配置：
   - PyTorch Build: **Stable**
   - Your OS: **Windows**
   - Package: **LibTorch**
   - Language: **C++**
   - Compute Platform: **CPU**
3. 下载 zip 文件
4. 解压到 `D:\tools\libtorch-cpu\`
5. 确保目录结构为 `D:\tools\libtorch-cpu\libtorch\`

### 步骤 4: 安装 VS Code 和扩展

1. 下载并安装 VS Code
2. 安装必需扩展：
   ```
   code --install-extension ms-vscode.cpptools-extension-pack
   code --install-extension ms-vscode.cmake-tools
   code --install-extension tonka3000.qtvsctools
   ```

## 🔧 项目配置

### 1. 克隆项目

```bash
git clone https://github.com/HankDai13/ai-devour-evolve.git
cd ai-devour-evolve
```

### 2. 配置环境变量（可选但推荐）

在系统环境变量中添加：
```
QT_DIR=D:\qt\6.9.1\msvc2022_64
LIBTORCH_ROOT=D:\tools\libtorch-cpu\libtorch
```

### 3. 验证 VS Code 配置

项目包含以下 VS Code 配置文件（如果被忽略，需要手动创建）：

**`.vscode/settings.json`** (需要根据你的路径调整):
```json
{
    "qtConfigure.qtKitDir": "D:/qt/6.9.1/msvc2022_64",
    "cmake.generator": "Visual Studio 17 2022",
    "cmake.platform": "x64",
    "cmake.preferredGenerators": [
        "Visual Studio 17 2022",
        "Visual Studio 16 2019",
        "MinGW Makefiles"
    ],
    "cmake.configureSettings": {
        "CMAKE_PREFIX_PATH": "D:/qt/6.9.1/msvc2022_64;D:/tools/libtorch-cpu/libtorch"
    }
}
```

**`.vscode/launch.json`**:
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "AI Devour Evolve (Debug)",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/Debug/ai-devour-evolve.exe",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "console": "integratedTerminal"
        }
    ]
}
```

### 4. 修改 CMakeLists.txt 路径

打开 `CMakeLists.txt`，确认或修改以下路径匹配你的安装：

```cmake
# 根据编译器选择Qt路径
if(MSVC)
    set(CMAKE_PREFIX_PATH "D:/qt/6.9.1/msvc2022_64;D:/tools/libtorch-cpu/libtorch")
elseif(MINGW)
    set(CMAKE_PREFIX_PATH "D:/qt/6.9.1/mingw_64;D:/tools/libtorch-cpu/libtorch")
endif()
```

## 🚀 构建和运行

### 方法 1: 使用 VS Code (推荐)

1. 打开项目文件夹: `File > Open Folder > ai-devour-evolve`
2. 按 `Ctrl+Shift+P` 打开命令面板
3. 输入 "CMake: Select a Kit"，选择 "Visual Studio Community 2022 Release - amd64"
4. 输入 "CMake: Configure" 配置项目
5. 按 `F7` 或输入 "CMake: Build" 构建项目
6. 按 `F5` 运行调试版本

### 方法 2: 使用命令行

```bash
# 配置项目
cmake -B build -S . -G "Visual Studio 17 2022" -A x64

# 构建项目
cmake --build build --config Debug

# 部署Qt依赖
D:\qt\6.9.1\msvc2022_64\bin\windeployqt.exe build\Debug\ai-devour-evolve.exe

# 运行程序
.\build\Debug\ai-devour-evolve.exe
```

## ✅ 验证安装

运行以下测试确保环境配置正确：

### 1. 验证 Qt 和 LibTorch

```bash
# 构建LibTorch测试程序
cmake -B build-test -S . -f CMakeLists-torch-only.txt -G "Visual Studio 17 2022" -A x64
cmake --build build-test --config Release

# 运行测试
.\build-test\Release\test-torch.exe
```

期望输出类似：
```
LibTorch version: 2.0.1
Random tensor:
 0.1234  0.5678  0.9012
 0.3456  0.7890  0.2345
[ CPUFloatType{2,3} ]
```

### 2. 验证主程序

运行主程序应该显示一个800x600的Qt窗口，标题为"智能吞噬进化 - 开发原型"。

## 🐛 常见问题

### Q1: "找不到 Qt6Core.dll"
**解决**: 运行 `windeployqt.exe` 部署Qt依赖库到exe所在目录。

### Q2: "找不到 torch.lib"
**解决**: 检查LibTorch路径是否正确，确保在 `D:\tools\libtorch-cpu\libtorch\` 目录下。

### Q3: "编译器不兼容"错误
**解决**: 确保Visual Studio 2022已安装，并在VS Code中选择正确的工具链。

### Q4: "CMake配置失败"
**解决**: 
1. 检查CMakeLists.txt中的路径
2. 确保Qt和LibTorch已正确安装
3. 尝试删除build目录重新配置

### Q5: VS Code无法找到头文件
**解决**: 
1. 按 `Ctrl+Shift+P` 输入 "C++: Edit Configurations"
2. 确保包含路径正确
3. 重启VS Code

## 📞 获取帮助

如果遇到问题：

1. 检查本文档的常见问题部分
2. 查看项目的 Issues 页面
3. 联系项目维护者

## 📝 注意事项

- **路径约定**: 本指南假设Qt安装在 `D:\qt\`，LibTorch安装在 `D:\tools\libtorch-cpu\`。如果安装在其他位置，请相应调整配置文件中的路径。
- **编译器选择**: 项目支持MSVC和MinGW，但强烈推荐使用MSVC以获得最佳兼容性。
- **LibTorch版本**: 请使用CPU版本的LibTorch，CUDA版本可能导致编译问题。
- **网络环境**: 某些步骤需要访问国外网站，可能需要科学上网。

---

**配置完成后，你就可以开始 AI Devour Evolve 的开发之旅了！** 🎮🚀
