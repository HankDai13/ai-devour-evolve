# 需求文档：实现C++核心逻辑库 (V2 - 包含AI接口规范)

**项目**: AI Devour Evolve

**目的**: 将游戏核心逻辑与Qt GUI完全解耦，创建一个可被Python调用、并严格遵循GoBigger实际数据格式的"无头"C++核心库。

## 最终成果

1. 一个高性能的、不依赖任何图形界面的C++游戏核心动态库 (`gobigger_core.dll`)
2. 一个可供Python import 并作为Gymnasium环境使用的模块
3. 原有的Qt图形化游戏程序能够继续运行，作为核心库的可视化前端

## 第一阶段：核心逻辑与GUI分离 (架构重构)

**目标**: 彻底剥离游戏逻辑对Qt QtWidgets 和 QtGui 的依赖，使其成为纯粹的数据和算法集合。

### 需求 1.1：创建纯数据状态类

**说明**: 当前所有球类 (BaseBall, CloneBall 等) 都继承自 QGraphicsObject，这是与GUI最紧密的耦合。我们需要将它们的状态和行为分离。

**实施步骤**:

1. 为每一个 `...Ball` 类创建一个对应的纯数据类，例如：
    - `BaseBallData`
    - `CloneBallData`
    - `FoodBallData`
    - `SporeBallData`
    - `ThornsBallData`

2. 这些 `...Data` 类 **不应** 继承任何Qt类

3. 将原 `...Ball` 类中的所有非GUI状态和逻辑迁移到 `...Data` 类中，包括：
    - **状态属性**: `m_score`, `m_radius`, `m_velocity`, `m_teamId`, `m_isRemoved` 等
    - **核心逻辑**: 物理更新 (`updatePhysics`)、移动 (`move`)、吞噬判断 (`canEat`, `eat`)、分裂/吐孢子逻辑等

4. 原有的 `...Ball` 类（QGraphicsObject子类）现在作为"渲染代理"：
    - 内部持有一个指向对应 `...Data` 对象的指针或引用
    - 职责简化为：在 `paint()` 方法中，读取 `...Data` 对象的状态（位置、半径、颜色）并进行绘制
    - 不再包含任何游戏逻辑

### 需求 1.2：创建核心游戏引擎 GameEngine

**说明**: GameManager 目前持有 QGraphicsScene，并使用 QTimer，这使其与GUI耦合。我们需要一个纯粹的引擎入口。

**实施步骤**:

1. 创建一个新的核心类 `GameEngine`，它不依赖任何GUI组件
2. `GameEngine` 将持有 `GameManager` 的实例，并成为与外界交互的主要接口
3. `GameEngine` 提供符合强化学习环境标准的接口：
    - `GameEngine::reset()`: 重置游戏状态
    - `GameEngine::step(const Action& action)`: 执行一个动作，并让游戏逻辑前进一帧
    - `GameEngine::getObservation() const`: 获取当前的游戏状态观察。此函数的输出必须严格遵循本文档第四阶段定义的AI接口规范
    - `GameEngine::isDone() const`: 判断游戏是否结束

4. `GameManager` 需要进行改造：
    - 移除 `QGraphicsScene* m_scene` 成员
    - 移除 `QTimer` 成员
    - 将 `updateGame()` 方法改为公开，由 `GameEngine::step()` 手动调用
    - 球体的创建和销毁不再直接操作 `QGraphicsScene`，而是通过内部的数据列表进行管理

### 需求 1.3：改造GUI应用

**说明**: 原有的Qt应用现在需要扮演"可视化客户端"的角色。

**实施步骤**:

1. `GameView` 不再创建 `GameManager`，而是创建 `GameEngine`
2. `GameView` 的 `QTimer` (即 `m_inputTimer`) 在触发时，不再直接调用游戏逻辑，而是：
    - 收集玩家输入（鼠标位置）
    - 调用 `m_gameEngine->step(action)`
    - 从 `m_gameEngine` 获取所有球体的最新状态
    - 更新场景中对应的 `QGraphicsObject`（即各个 `...Ball` 对象）的位置和外观
3. 创建和销毁 `QGraphicsObject` 的逻辑将基于 `GameEngine` 内部状态的变化

## 第二阶段：构建系统分离 (CMake改造)

**目标**: 修改CMakeLists.txt，使其能够同时编译出核心逻辑库和GUI应用两个产物。

### 需求 2.1：定义核心逻辑库目标

**实施步骤**:

1. 在CMakeLists.txt中，创建一个新的 `SHARED LIBRARY` 目标，命名为 `gobigger_core`
2. 此目标的源文件应包含所有 `...Data` 类、`GameEngine`、`GameManager` 和 `QuadTree` 的 .h 和 .cpp 文件
3. `gobigger_core` 目标 **只能** 链接到 `Qt6::Core`（如果使用了QPointF等），**绝不能** 链接 `Qt6::Widgets` 或 `Qt6::Gui`

### 需求 2.2：修改GUI应用目标

**实施步骤**:

1. 修改原有的 `add_executable(ai-devour-evolve ...)` 目标
2. 其源文件列表现在只应包含 `main.cpp`, `GameView.cpp`, 以及所有 `QGraphicsObject` 渲染代理类 (`BaseBall.cpp`, `CloneBall.cpp` 等)
3. 此目标需要链接到上一步创建的 `gobigger_core` 库，以及 `Qt6::Widgets`

## 第三阶段：创建Python绑定

**目标**: 使用pybind11为gobigger_core库创建Python接口，使其可以作为标准的Python模块使用。

### 需求 3.1：集成pybind11

**实施步骤**:

1. 将pybind11和pybind11_qt（用于Qt类型转换）作为子模块或通过FetchContent添加到项目中
2. 在CMakeLists.txt中创建一个新的目标，用于编译Python模块

### 需求 3.2：编写绑定代码

**实施步骤**:

1. 创建一个新的源文件，例如 `python/bindings.cpp`
2. 使用 `PYBIND11_MODULE` 宏定义一个Python模块
3. 绑定 `GameEngine` 类及其公共方法 (`reset`, `step`, `getObservation`, `isDone`)
4. 绑定用于 `step` 和 `getObservation` 的数据结构（如Action和Observation结构体）
5. 包含 `pybind11_qt/pybind11_qt.h` 头文件，以自动处理 `QPointF`、`QVector2D` 等QtCore类型与Python原生类型（元组、列表）之间的转换

### 需求 3.3：验证Python模块

**实施步骤**:

1. 编译生成Python模块（例如 `gobigger_env.pyd`）
2. 编写一个 `test_env.py` 脚本，尝试import该模块，并执行以下操作：

```python
import gobigger_env

env = gobigger_env.GameEngine()
obs = env.getObservation()
print("Initial Observation:", obs)
action = [0.5, -0.5, 0] # 示例动作
env.step(action)
new_obs = env.getObservation()
print("Observation after one step:", new_obs)
```

3. 确保脚本能成功运行并打印出正确的状态信息

## 第四阶段：AI接口实现与数据对齐

**目标**: 确保 `GameEngine::getObservation()` 和 `GameEngine::step()` 的数据格式与GoBigger训练环境完全一致。

### 需求 4.1：观察空间 (Observation Space)

C++ GameEngine 必须生成与以下结构完全匹配的数据。

**顶层结构**: 返回一个包含两个主要部分的结构或字典：`global_state` 和 `player_states`。

**全局状态 (global_state)**:
- `border`: [map_width, map_height]
- `total_frame`: int
- `last_frame_count`: int
- `leaderboard`: map<int, float> (队伍ID -> 分数)

**玩家状态 (player_states)**:
一个以 `player_id` 为键的map。每个玩家的值包含：
- `rectangle`: [x_min, y_min, x_max, y_max] (玩家视野矩形)
- `overlap`: 包含 `food`, `thorns`, `spore`, `clone` 四个列表的结构
- `score`: float
- `can_eject`, `can_split`: bool

### 需求 4.2：视野内对象格式 (overlap Objects)

C++ GameEngine 必须以列表/向量的形式提供视野内的对象，且每个对象的元素顺序和类型必须严格匹配。

- **食物 (food)**: [x, y, radius, score]
- **荆棘 (thorns)**: [x, y, radius, score, vx, vy]
- **孢子 (spore)**: 格式待定，目前可为空列表
- **克隆球 (clone)**: [x, y, radius, score, vx, vy, dir_x, dir_y, team_id, player_id]

### 需求 4.3：数据预处理

`GameEngine::getObservation()` 在输出最终的扁平化向量前，必须执行与Python训练时完全一致的预处理。

**数量限制与填充/裁剪**:
- 食物: 最多50个
- 荆棘: 最多20个
- 孢子: 最多10个
- 克隆球: 最多30个
- 不足时用0填充，超出时按距离裁剪

**归一化**:
- 位置: 除以地图尺寸
- 半径/分数: 除以预设的最大值（如半径/100, 分数/1000）
- 时间: 当前帧数 / 总帧数

**特征提取**:
最终为每个对象提取一个固定维度的特征向量（例如，10维），包含one-hot编码的类型、相对坐标、半径、分数、关系（是否为自己/队友）等。

### 需求 4.4：动作空间 (Action Space)

C++ `GameEngine::step()` 方法必须能接收并解析以下格式的动作。

**输入格式**: 一个包含3个float元素的列表/向量 `[direction_x, direction_y, action_type]`。

**解析逻辑**:
- `direction_x`, `direction_y`: 裁剪到 [-1.0, 1.0] 范围
- `action_type`: 裁剪到 [0, 2] 范围，然后转换为int（0: 无动作, 1: 吐球, 2: 分裂）