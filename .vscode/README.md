# VS Code 配置模板

本目录包含了项目的VS Code配置文件模板，协作者需要根据自己的环境调整这些配置。

## 配置文件说明

### settings.json
包含Qt路径、CMake生成器和LibTorch路径配置。请根据你的安装路径修改：

```json
{
    "qtConfigure.qtKitDir": "你的Qt安装路径/6.9.1/msvc2022_64",
    "cmake.configureSettings": {
        "CMAKE_PREFIX_PATH": "你的Qt路径;你的LibTorch路径"
    }
}
```

### launch.json
包含调试配置，通常不需要修改。

### 其他文件
- `c_cpp_properties.json` - 自动生成，已忽略
- `tasks.json` - 自动生成，已忽略

## 快速配置

1. 按照 `environment-setup-guide.md` 安装所需软件
2. 克隆项目后，VS Code 会自动使用这些配置
3. 如果路径不匹配，请修改 `settings.json` 中的路径
