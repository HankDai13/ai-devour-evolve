# Git协作流程文档

## 1. 宗旨与目标
本指南旨在为项目提供一套清晰、简单、高效的协作流程。

## 2. 角色与分工 
- **队员A (HankDai13)**: 
  - Python AI训练总负责人
  - C++项目架构师与AI集成工程师
- **队员B**: 
  - C++网络与实现工程师

## 3. Git与GitHub工作流

### 3.1 分支管理规则
- `main` 分支是神圣的：任何人都不能直接向 main 分支提交代码
- 所有新工作都在新分支上进行
- 分支命名规范：
  - 开发新功能：`feature/功能描述`
  - 修复Bug：`bugfix/问题描述`

### 3.2 日常开发流程

#### 基本工作流（每个功能都要重复）

1. **同步最新代码**
```bash
git checkout main
git pull origin main
```

2. **创建功能分支**
```bash
git checkout -b feature/player-movement
```

3. **编码与提交**
```bash
git add .
git commit -m "feat: Implement basic player movement logic"
```

4. **推送分支**
```bash
git push origin feature/player-movement
```

5. **创建Pull Request**
- 在GitHub上创建PR
- 指定审查者
- 等待代码审查

6. **代码审查与合并**
- 审查者检查代码
- 修改问题后重新提交
- 审查通过后合并到main

7. **清理**
```bash
git checkout main
git branch -d feature/player-movement
```

#### 开发阶段的详细工作流

**开始新功能开发时：**
```bash
# 1. 确保在main分支并同步最新代码
git checkout main
git pull origin main

# 2. 创建新的功能分支
git checkout -b feature/你的功能描述

# 3. 确认分支创建成功
git branch  # 应该显示 * feature/你的功能描述
```

**开发过程中的提交循环：**
```bash
# 1. 编写代码，添加新文件，修改现有文件...

# 2. 当完成一个小功能或重要修改后，提交：
git add .  # 或者 git add 具体文件名
git commit -m "feat: 描述你完成的功能"

# 3. 继续开发更多功能...
# 4. 再次提交：
git add .
git commit -m "feat: 描述下一个功能"

# 5. 定期推送保存进度（推荐每天至少一次）
git push origin feature/你的功能描述
```

**同步main分支更新（如果有协作者提交）：**
```bash
# 当main分支有新提交时，同步到你的功能分支：
git checkout main
git pull origin main
git checkout feature/你的功能描述
git merge main  # 或者使用 git rebase main（更干净的历史）
```

**完成功能开发：**
```bash
# 最终推送
git push origin feature/你的功能描述

# 然后在GitHub上创建Pull Request
# 标题：feat: 功能名称
# 描述：详细说明你实现的功能
# 指定协作者为审查者
```

#### 提交信息规范
按照约定使用以下前缀：
- `feat:` - 新功能
- `fix:` - 修复Bug  
- `refactor:` - 代码重构
- `docs:` - 文档更新
- `test:` - 添加测试
- `style:` - 代码格式化（不影响功能）

**示例：**
```bash
git commit -m "feat: Add Game core class structure

- Create Game.h/cpp with basic game loop
- Define Player and Cell base classes  
- Add GameState structure for AI interface
- Implement basic collision detection framework"
```

### 3.3 项目初期特殊流程

**协作者环境配置阶段：**
1. **队员A** 在main分支上提供稳定的"环境验证"版本
2. **队员B** 按照 `develop-Documents/environment-setup-guide.md` 配置环境
3. **队员B** 成功运行当前Qt窗口程序作为环境验证
4. 环境配置完成后，开始使用标准分支工作流

**架构开发与环境配置并行：**
- **队员A** 在 `feature/game-core-architecture` 分支开发框架
- **队员B** 在配置环境，目标是运行main分支的程序
- 当队员B环境配好后，队员A创建PR合并架构到main
- 然后开始正常的分支协作流程

## 4. 接口约定

### 4.1 AI模型文件格式
- 格式：`.pt` 或 `.pth`
- 位置：`models/` 目录

### 4.2 状态空间约定
```cpp
// 向量维度：38
[
  self_x,              // 自身x坐标
  self_y,              // 自身y坐标  
  self_radius,         // 自身半径
  food_1_x, food_1_y,  // 最近10个食物的相对坐标 (20个值)
  ...
  enemy_1_x, enemy_1_y, enemy_1_radius_diff, // 最近5个敌人信息 (15个值)
  ...
]
```

### 4.3 动作空间约定
```cpp
// 向量维度：2
[
  target_x_direction,  // x方向移动 (-1 到 1)
  target_y_direction   // y方向移动 (-1 到 1)
]
```

## 5. 沟通与任务管理
- **任务板**: GitHub Issues
- **沟通渠道**: 微信群或Discord
- **定期同步**: 每2-3天语音同步进度

## 6. 常见Git场景处理

### 6.1 忘记创建分支直接在main上修改了
```bash
# 不要panic！创建新分支保存你的修改
git checkout -b feature/你的功能描述
git push origin feature/你的功能描述

# 然后重置main分支
git checkout main
git reset --hard origin/main
```

### 6.2 需要临时切换分支处理紧急问题
```bash
# 暂存当前工作
git stash

# 切换到其他分支处理问题
git checkout main
# 处理问题...

# 回到原分支继续工作
git checkout feature/原来的分支
git stash pop  # 恢复之前的工作
```

### 6.3 合并冲突处理
```bash
# 当merge时出现冲突：
git merge main
# Auto-merging src/Game.cpp
# CONFLICT (content): Merge conflict in src/Game.cpp

# 1. 手动编辑冲突文件，解决冲突
# 2. 标记冲突已解决
git add src/Game.cpp
# 3. 完成合并
git commit -m "resolve: Merge conflict in Game.cpp"
```

### 6.4 撤销最后一次提交（但保留修改）
```bash
git reset --soft HEAD~1  # 撤销提交，保留文件修改
git reset HEAD~1         # 撤销提交和暂存，保留文件修改
git reset --hard HEAD~1  # 完全撤销（慎用！）
```

## 7. 最佳实践

### 7.1 提交频率
- **频繁小提交** 比大提交好
- 每个提交应该是一个逻辑完整的功能点
- 每天至少push一次保存进度

### 7.2 分支管理
- 功能分支应该短寿命（1-3天完成）
- 及时删除已合并的本地分支
- 定期同步main分支到功能分支

### 7.3 代码审查
- 审查者应在24小时内响应
- 提供建设性的反馈，不仅仅是指出问题
- 小的修改可以在PR中直接建议

### 7.4 紧急情况处理
如果遇到紧急Bug需要立即修复：
```bash
# 创建hotfix分支
git checkout main
git checkout -b hotfix/紧急修复描述

# 修复后立即合并，跳过常规审查流程
git checkout main
git merge hotfix/紧急修复描述
git push origin main

# 通知所有协作者立即同步
```
