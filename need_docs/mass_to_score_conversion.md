# Mass到Score体系全面转换完成

## 概述
已完成从质量(mass)体系到分数(score)体系的全面重构，完全对齐GoBigger原版标准。

## 主要变更

### 1. 配置参数更新 (GoBiggerConfig.h)
- `CELL_MIN_MASS` → `CELL_MIN_SCORE = 1000` (GoBigger标准)
- `CELL_INIT_MASS` → `CELL_INIT_SCORE = 1000` (GoBigger标准)
- `CELL_MAX_MASS` → `CELL_MAX_SCORE = 50000`
- `SPLIT_MIN_MASS` → `SPLIT_MIN_SCORE = 3600` (GoBigger标准)
- `EJECT_MIN_MASS` → `EJECT_MIN_SCORE = 3200` (GoBigger标准)
- `EJECT_MASS` → `EJECT_SCORE = 1400` (GoBigger标准)
- `FOOD_MASS` → `FOOD_SCORE = 100` (GoBigger标准)
- `FOOD_MIN_MASS/MAX_MASS` → `FOOD_MIN/MAX_SCORE = 100` (固定分数)
- `THORNS_MIN/MAX_MASS` → `THORNS_MIN/MAX_SCORE = 10000/15000` (GoBigger标准)
- `DECAY_START_MASS` → `DECAY_START_SCORE = 2600.0f`
- `EAT_RATIO = 1.3f` (从1.25f更新为GoBigger标准)

### 2. 核心函数更新
- `massToRadius()` → `scoreToRadius()` - 使用GoBigger原版公式：`sqrt(score / 100 * 0.042 + 0.15)`
- `radiusToMass()` → `radiusToScore()` - 反向计算
- `canEat(eaterMass, targetMass)` → `canEat(eaterScore, targetScore)`
- `canSplit(mass, count)` → `canSplit(score, count)`
- `canEject(mass)` → `canEject(score)`

### 3. BaseBall类更新
- `m_mass` → `m_score`
- `mass()` → `score()`
- `setMass()` → `setScore()`
- `massChanged` 信号 → `scoreChanged` 信号
- `updateRadius()` 使用新的 `scoreToRadius()` 函数

### 4. 所有球类更新
#### CloneBall
- 构造函数使用 `CELL_INIT_SCORE`
- `canSplit()` 检查 `SPLIT_MIN_SCORE`
- `canEject()` 检查 `EJECT_MIN_SCORE`
- 分裂逻辑使用分数计算
- 孢子喷射使用 `EJECT_SCORE`
- 合并逻辑使用分数加权平均
- 衰减系统使用 `DECAY_START_SCORE`

#### FoodBall
- 使用 `FOOD_SCORE = 100` (GoBigger标准)

#### SporeBall
- 使用 `EJECT_SCORE = 1400` (GoBigger标准)

#### ThornsBall
- 使用 `THORNS_MIN/MAX_SCORE` 范围
- 伤害计算基于分数

### 5. UI更新
- 状态栏显示"分数"而不是"质量"
- 所有调试输出更新为score术语

### 6. 游戏逻辑更新
- 碰撞检测使用分数比较
- 吞噬判定使用新的1.3倍比例
- 所有质心计算使用分数加权
- 物理计算基于新的半径公式

## GoBigger对齐验证

### 分数标准
✅ 玩家初始分数：1000  
✅ 食物分数：100  
✅ 孢子分数：1400  
✅ 荆棘分数：10000-15000  
✅ 分裂最小分数：3600  
✅ 喷射最小分数：3200  
✅ 吞噬比例：1.3倍  

### 半径计算
✅ 使用GoBigger原版公式：`radius = sqrt(score / 100 * 0.042 + 0.15)`

### 游戏机制
✅ 所有球类行为基于分数而非质量  
✅ 物理计算适配新的半径公式  
✅ UI显示正确的分数信息  

## 编译测试
- ✅ 编译成功，无错误
- ✅ 运行正常
- ⚠️ 一个float截断警告（不影响功能）

## 文件变更清单
1. `GoBiggerConfig.h` - 全面重写配置参数
2. `BaseBall.h/cpp` - 核心类更新为score体系
3. `CloneBall.cpp` - 所有玩家球逻辑更新
4. `FoodBall.cpp` - 食物球分数标准化
5. `SporeBall.cpp` - 孢子球分数标准化
6. `ThornsBall.cpp` - 荆棘球分数和伤害逻辑更新
7. `GameManager.cpp` - 碰撞检测和调试输出更新
8. `GameView.cpp` - UI和游戏逻辑更新
9. `main.cpp` - 状态栏显示更新

## 后续优化建议
1. 可以考虑微调一些数值以优化游戏体验
2. 可以添加更多GoBigger原版特性
3. 可以优化性能和视觉效果

此次转换彻底实现了从mass到score体系的迁移，完全对齐GoBigger原版标准。
