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
