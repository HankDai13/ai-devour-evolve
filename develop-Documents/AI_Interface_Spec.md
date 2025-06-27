# ai-devour-evolve AI 接口规范 (C++ 实现) - 详细版

本文档基于 Python GoBigger 项目的接口，定义了 AI 模型的观察空间 (Observation Space) 和动作空间 (Action Space)。ai-devour-evolve 的 C++ 实现必须严格遵循这些规范，以确保与 Python 训练的强化学习模型兼容。

## 1. 观察空间 (Observation Space)

AI 模型将在每个模拟步骤接收一个观察数据。这个观察数据是游戏状态的结构化表示，为 AI 做出决策提供必要的信息。

GoBigger 的 `server.py` 中的 `obs()` 方法返回一个元组 `(global_state, player_states, info)`。C++ 实现需要将这些数据转换为 LibTorch 模型可接受的张量格式。

### 1.1 全局状态 (Global State)

对应 GoBigger `server.py` 中 `get_global_state()` 方法返回的字典。

*   **`border`**: `list[float, float]` - 地图的宽度和高度。
    *   `[map_width, map_height]`
    *   示例：`[1000.0, 1000.0]`
*   **`total_frame`**: `int` - 模拟的总帧数限制。
    *   示例：`36000`
*   **`last_frame_count`**: `int` - 当前模拟帧数。
    *   示例：`1234`
*   **`last_time`**: `int` - 同 `last_frame_count`，用于兼容性。
    *   示例：`1234`
*   **`leaderboard`**: `dict[int, float]` - 队伍分数排行榜。
    *   键是队伍 ID (`int`)，值是该队伍的总分数 (`float`)。
    *   示例：`{0: 1500.5, 1: 1200.0}`

### 1.2 玩家特定状态 (Player-Specific State)

对应 GoBigger `server.py` 中 `player_states_util.get_player_states()` 方法返回的字典。这个字典的键是玩家 ID (`str`)，值是该玩家的观察数据。

**对于每个 AI 控制的玩家（即每个 `player_id`），其观察数据字典包含以下键值对：**

*   **`player_id`**: `str` - 玩家的唯一 ID。
    *   示例：`'player_0'`
*   **`team_id`**: `int` - 玩家所属队伍的 ID。
    *   示例：`0`
*   **`ball_id`**: `str` - 玩家当前控制的球的 ID (通常是最大的那个)。
    *   示例：`'ball_123'`
*   **`ball_num`**: `int` - 玩家当前控制的克隆球数量。
    *   示例：`5`
*   **`player_score`**: `float` - 玩家的总分数。
    *   示例：`350.7`
*   **`team_score`**: `float` - 玩家所属队伍的总分数。
    *   示例：`1500.5`
*   **`overlap`**: `dict` - 玩家视野范围内的所有对象。
    *   这是一个嵌套字典，键是对象类型（`'food'`, `'thorns'`, `'spore'`, `'clone'`），值是该类型对象的列表。
    *   **C++ 实现需要根据玩家当前最大的克隆球的半径和配置的视野缩放比例来计算视野范围。** GoBigger 在 `utils/obs_utils.py` 中使用 `PlayerStatesUtil` 来处理部分视野 (`obs_settings.partial.vision_x_min`, `vision_y_min`, `scale_up_ratio`)。
    *   **每个对象（食物、刺球、孢子、克隆球）的详细结构如下：**
        *   **通用属性 (所有对象都有):**
            *   `ball_id`: `str` - 对象的唯一 ID。
            *   `position`: `list[float, float]` - 对象的中心坐标 `[x, y]`。
            *   `radius`: `float` - 对象的半径。
            *   `score`: `float` - 对象的分数/质量。
        *   **克隆球特有属性 (仅 `overlap['clone']` 中的对象有):**
            *   `player_id`: `str` - 克隆球所属玩家的 ID。
            *   `team_id`: `int` - 克隆球所属队伍的 ID。
            *   `speed`: `list[float, float]` - 克隆球的当前速度向量 `[vx, vy]`。
            *   `is_self`: `bool` - 是否是当前 AI 玩家自身的克隆球。
            *   `is_team`: `bool` - 是否是当前 AI 玩家队友的克隆球 (如果 `is_self` 为 `False`)。
            *   `is_thorn_eaten`: `bool` - 是否被刺球吃掉 (用于回放，推理时通常为 `False`)。
            *   `is_spore_eaten`: `bool` - 是否被孢子吃掉 (用于回放，推理时通常为 `False`)。
            *   `is_clone_eaten`: `bool` - 是否被其他克隆球吃掉 (用于回放，推理时通常为 `False`)。

### 1.3 数据归一化和填充 (Data Normalization and Padding)

*   **归一化：** 所有数值（位置、半径、分数、速度）在传递给 AI 模型之前，都应归一化或缩放到一个一致的范围（例如，`[0, 1]` 或 `[-1, 1]`）。
    *   **位置：** 可以除以 `map_width` 和 `map_height`。
    *   **半径/分数：** 可以除以最大可能半径/分数，或使用对数缩放。
    *   **速度：** 可以除以最大速度。
    *   **C++ 端必须与 Python 训练端使用完全相同的归一化方法。**
*   **填充/裁剪：** `overlap` 字典中的列表（`food`, `thorns`, `spore`, `clone`）是动态大小的。为了神经网络的固定大小输入，C++ 端需要：
    *   **定义最大数量：** 为每种类型的对象定义一个最大数量（例如，视野内最多 50 个食物，20 个刺球，10 个孢子，30 个克隆球）。
    *   **填充：** 如果实际数量少于最大数量，用零向量或预定义的虚拟值填充。
    *   **裁剪：** 如果实际数量多于最大数量，通常选择距离玩家最近的对象。

### 1.4 观察张量结构 (Tensor Structure)

最终，C++ 端需要将上述所有数据组合成一个或多个 `torch::Tensor`，作为 LibTorch 模型的输入。具体的张量形状和内容顺序需要与 Python 训练模型的输入层精确匹配。

**建议的张量结构（示例，具体需要根据你的模型输入层设计）：**

*   **全局状态张量:** `torch::Tensor global_obs` (例如，`[1, 4 + num_teams]`，包含地图尺寸、帧数、排行榜)
*   **玩家自身球张量:** `torch::Tensor self_balls_obs` (例如，`[max_self_balls, 6]`，包含位置、半径、分数、速度)
*   **视野内对象张量:** `torch::Tensor overlap_obs` (例如，`[max_overlap_objects, 10]`，包含类型、位置、半径、分数、速度、玩家ID、队伍ID等)
    *   `object_type` 可以用 One-Hot 编码或直接映射为整数。
    *   所有 ID (ball_id, player_id) 通常不直接作为数值输入，而是用于索引或分类。

## 2. 动作空间 (Action Space)

AI 模型将输出动作，C++ 实现负责从 LibTorch 模型接收这些动作并将其应用于游戏。

GoBigger 的 `step_one_frame()` 方法接收一个 `actions` 字典，其键是玩家 ID，值是 `[direction_x, direction_y, action_type]`。

**对于每个 AI 控制的玩家，模型将输出以下动作：**

*   **移动方向 (Movement Direction):**
    *   `direction_x`: `float` - 期望移动向量的 X 分量。
    *   `direction_y`: `float` - 期望移动向量的 Y 分量。
    *   这些值通常是归一化的（例如，在 `[-1.0, 1.0]` 范围内，表示单位向量）。C++ 游戏引擎将根据玩家的当前速度和这个方向来更新玩家球的位置。
*   **动作类型 (Action Type):**
    *   `action_type`: `int` - 离散动作 ID。
        *   `0`: 无特殊动作（仅移动）。
        *   `1`: 吐球 (eject)。
        *   `2`: 分裂 (split)。
    *   AI 模型将输出一个整数值。

**C++ 端需要将 LibTorch 模型输出的张量解析为这些浮点数和整数，并将其应用于游戏逻辑。**

## 3. 模型导出和导入 (LibTorch)

*   **Python (训练)：** Python 训练脚本将使用 PyTorch 训练强化学习模型。训练完成后，模型将使用 `torch.jit.trace` 或 `torch.jit.script` 导出为 `TorchScript` 文件（`.pt` 或 `.pth` 扩展名）。
*   **C++ (推理)：** C++ `ai-devour-evolve` 项目将使用 LibTorch 加载导出的 `TorchScript` 模型。C++ 代码将：
    1.  根据第 1 节构建观察张量。
    2.  将观察张量作为输入传递给加载的 LibTorch 模型。
    3.  从模型接收动作张量作为输出。
    4.  根据第 2 节解释并将动作应用于游戏状态。

## 4. C++ 实现注意事项

*   **数据结构：** 考虑使用 `std::vector` 或自定义 C++ 结构体/类来表示游戏对象和观察，然后再将其转换为 `torch::Tensor` 作为模型输入。
*   **视野计算：** C++ 端需要实现高效的四叉树或其他空间数据结构来快速查询视野内的对象。
*   **效率：** 观察生成和动作应用逻辑应高度优化，以实现实时性能。
*   **错误处理：** 对模型加载及其输入/输出处理实施健壮的错误处理。
*   **同步：** 确保用于观察生成的游戏状态与游戏的更新循环保持一致和同步。

---
**概念性数据流示例：**

1.  **C++ 游戏循环：**
    *   `GameManager` 更新游戏状态。
    *   `GameManager`（或专门的 `ObservationGenerator` 类）为每个 AI 玩家收集 `global_state` 和 `player_specific_state`。
    *   这些状态根据第 1 节的规范进行归一化、填充，并转换为 `torch::Tensor` 对象。
    *   `torch::Tensor observation_input = ...;`
    *   `torch::Tensor action_output = model.forward({observation_input}).toTensor();`
    *   `direction_x = action_output[0].item<float>();`
    *   `direction_y = action_output[1].item<float>();`
    *   `action_type = action_output[2].item<int>();`
    *   `GameManager` 将 `direction_x`、`direction_y`、`action_type` 应用于玩家。

---
**修订历史：**

*   2025年6月27日: Gemini Agent 初稿
*   2025年6月27日: Gemini Agent 详细版修订