# 分支切换和集成工作指南

## 🚀 当前状态总结

### RL训练分支成果 ✅
- **Gemini优化完成**: 修复策略崩塌问题，优化PPO超参数
- **事件驱动奖励**: 增加Split(+2.0)和Eject(+1.5)动作激励
- **模型导出成功**: 生成ONNX和TorchScript格式的AI模型
- **训练系统稳定**: Rich界面、长时间训练支持、自动保存

### 已导出模型
```
exported_models/
├── ai_model.onnx              # ONNX格式 (4.83 MB) - 推荐C++集成
├── ai_model_traced.pt         # TorchScript格式 (4.87 MB) 
├── model_info.json            # 模型详细信息
└── AIModelInterface.h         # C++集成接口定义
```

## 📋 分支切换步骤

### 1. 保存当前RL训练进度
```bash
# 提交当前所有更改
git add .
git commit -m "🔥 Gemini优化完成: 修复策略崩塌+事件驱动奖励+模型导出

✅ 完成内容:
- 降低n_epochs防止过拟合 (20→6-8)
- 提高ent_coef增加探索多样性 (0.005→0.01+)
- 添加Split/Eject动作事件奖励系统
- 导出ONNX和TorchScript格式AI模型
- Rich界面优化和长时间训练支持

🎯 下一步: 切换到AI集成分支开始多玩家系统开发"

# 推送到远程(如果需要)
git push origin rl-training-branch
```

### 2. 切换到main并创建新分支
```bash
# 切换到main分支
git checkout main

# 创建AI集成分支
git checkout -b ai-integration-dev

# 确认分支状态
git branch
```

### 3. 拷贝必要的模型文件
```bash
# 创建AI模型目录
mkdir -p assets/ai_models

# 从RL分支拷贝导出的模型文件
# (需要先切回RL分支拷贝，或者直接从工作目录拷贝)
cp exported_models/ai_model.onnx assets/ai_models/
cp exported_models/model_info.json assets/ai_models/
cp exported_models/AIModelInterface.h src/ai/
```

## 🎯 AI集成分支开发计划

### 第一阶段: AI模型集成 (Week 1)
**目标**: 让C++端能够加载和使用导出的AI模型

#### 核心任务
1. **ONNX Runtime集成**
   ```cmake
   # CMakeLists.txt 添加
   find_package(onnxruntime REQUIRED)
   target_link_libraries(gobigger_core onnxruntime)
   ```

2. **AI推理类实现**
   ```cpp
   // 基于AIModelInterface.h实现
   class ONNXInference : public ModelInference {
       // 实现模型加载和推理
   };
   ```

3. **AIPlayer类开发**
   ```cpp
   class AIPlayer : public Player {
       std::unique_ptr<ModelInference> ai_model;
       // 决策循环和动作执行
   };
   ```

#### 验证方式
- [ ] C++端能加载ONNX模型
- [ ] AI推理输出与Python端一致
- [ ] AIPlayer能在游戏中正常移动

### 第二阶段: 多玩家功能开发 (Week 2)
**目标**: 实现完整的多玩家对战系统

#### 核心任务
1. **玩家互动机制**
   ```cpp
   class PlayerInteractionSystem {
       bool checkCollision(Player& p1, Player& p2);
       void handleDevour(Player& larger, Player& smaller);
       void processSplitAttack(Player& attacker, Player& target);
   };
   ```

2. **多玩家游戏管理**
   ```cpp
   class MultiPlayerGame {
       std::vector<std::unique_ptr<Player>> players;
       void addAIPlayer(const std::string& model_path);
       void addHumanPlayer(int player_id);
   };
   ```

#### 功能要求
- [ ] 2-8个玩家同时游戏
- [ ] 玩家间吞噬功能
- [ ] Split分裂攻击
- [ ] Eject喷射互动

### 第三阶段: 调试和优化 (Week 3-4)
**目标**: 完善系统稳定性和用户体验

#### 调试工具
- [ ] AI行为可视化
- [ ] 多玩家状态监控
- [ ] 性能分析工具

#### 用户界面
- [ ] 玩家排行榜显示
- [ ] AI难度设置
- [ ] 观战模式

## 🔧 技术架构设计

### 依赖库规划
```cmake
# 新增依赖
find_package(onnxruntime REQUIRED)  # AI推理
find_package(OpenCV OPTIONAL)       # 可视化(可选)
find_package(ImGui OPTIONAL)        # 调试界面(可选)
```

### 文件结构
```
src/
├── ai/                     # AI相关代码
│   ├── AIPlayer.h/cpp
│   ├── ModelInference.h/cpp
│   └── AIDebugger.h/cpp
├── multiplayer/            # 多玩家系统
│   ├── MultiPlayerGame.h/cpp
│   ├── PlayerManager.h/cpp
│   └── InteractionSystem.h/cpp
├── debug/                  # 调试工具
│   └── GameDebugger.h/cpp
└── ui/                     # 用户界面
    └── MultiPlayerUI.h/cpp

assets/
└── ai_models/              # AI模型文件
    ├── ai_model.onnx
    └── model_info.json
```

### 性能考虑
1. **AI推理优化**
   - 异步推理避免阻塞主线程
   - 批量推理提高效率
   - 内存池管理

2. **渲染优化**
   - 多玩家场景LOD
   - 视锥裁剪
   - 实例化渲染

## 📊 验收标准

### 功能验收
- [ ] 4个AI同时对战运行流畅
- [ ] AI使用Split/Eject等高级动作
- [ ] 玩家间互动(吞噬)正常
- [ ] 游戏帧率 ≥ 30FPS

### 质量验收
- [ ] 无内存泄漏
- [ ] 异常处理完善
- [ ] 代码结构清晰
- [ ] 文档齐全

## 🚦 风险控制

### 主要风险
1. **ONNX集成复杂度**: C++端ONNX Runtime配置可能复杂
2. **性能瓶颈**: 多AI实时推理可能影响性能
3. **兼容性问题**: 不同平台的库依赖

### 风险缓解
1. **逐步集成**: 先实现基础功能，再添加复杂特性
2. **性能监控**: 每个阶段都进行性能测试
3. **跨平台测试**: 早期在不同环境测试

## 📝 提交规范

### 分支命名
- `ai-integration-dev`: 主开发分支
- `feature/ai-inference`: AI推理功能
- `feature/multiplayer-core`: 多玩家核心
- `feature/debug-tools`: 调试工具

### 提交格式
```
<type>: <subject>

<body>

<footer>
```

例如:
```
feat: 实现ONNX模型推理接口

- 添加ONNXInference类
- 实现模型加载和推理方法  
- 单元测试覆盖主要功能

Closes #123
```

## 🎉 最终目标

构建一个完整的AI驱动的多玩家GoBigger游戏系统，具备：
- **智能AI对手**: 使用Gemini优化的强化学习模型
- **流畅多玩家体验**: 支持人类vs AI或AI vs AI对战
- **丰富互动机制**: 完整的吞噬、分裂、喷射系统
- **强大调试工具**: 便于开发和优化

让我们开始这个激动人心的AI集成之旅！🚀
