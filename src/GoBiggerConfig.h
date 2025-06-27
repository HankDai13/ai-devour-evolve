#ifndef GOBIGGERCONFIG_H
#define GOBIGGERCONFIG_H

#include <QColor>
#include <QVector>
#include <cmath>

// GoBigger标准配置参数
namespace GoBiggerConfig {

// ============ 核心数值参数 ============

// 细胞基础参数 (参考GoBigger原版score体系)
constexpr float CELL_MIN_RADIUS = 10.0f;           // 最小半径
constexpr float CELL_MAX_RADIUS = 300.0f;          // 最大半径
constexpr int CELL_MIN_SCORE = 1000;               // 最小分数 (GoBigger标准)
constexpr int CELL_INIT_SCORE = 1000;              // 玩家初始分数 (GoBigger标准)
constexpr int CELL_MAX_SCORE = 50000;              // 最大分数
constexpr float RADIUS_DISPLAY_SCALE = 20.0f;      // 半径显示缩放因子（让球体大小合适）

// 移动参数 (基于GoBigger动态计算)
constexpr float BASE_SPEED = 300.0f;               // 基础移动速度 (参考文档200-300)
constexpr float SPEED_DECAY_FACTOR = 1.0f;         // 速度衰减因子
constexpr float ACCELERATION_FACTOR = 1.0f;        // 加速度因子 (acc_weight=30)
constexpr float SPEED_RADIUS_COEFF_A = 50.00f;      // 速度计算系数A
constexpr float SPEED_RADIUS_COEFF_B = 80.00f;      // 速度计算系数B
// 实际速度 = (2.35 + 5.66 / radius) * ratio

// 分裂参数 (参考GoBigger原版)
constexpr int SPLIT_MIN_SCORE = 3600;              // 最小分裂分数 (GoBigger标准)
constexpr int MAX_SPLIT_COUNT = 16;                // 最大分裂数量
constexpr float SPLIT_BOOST_SPEED = 500.0f;        // 分裂冲刺速度 (文档标准)
constexpr float SPLIT_COOLDOWN = 1.0f;             // 分裂冷却时间(秒)
constexpr float MERGE_DELAY = 20.0f;               // 合并延迟时间(秒) (recombine_frame=400/20fps)

// 吞噬参数 (参考GoBigger原版)
constexpr float EAT_RATIO = 1.3f;                  // 吞噬分数比例 (GoBigger标准)
constexpr float EAT_DISTANCE_RATIO = 0.8f;         // 吞噬距离比例

// 吐孢子参数 (参考GoBigger原版)
constexpr int EJECT_SCORE = 1400;                  // 孢子分数 (GoBigger标准)
constexpr float EJECT_SPEED = 400.0f;              // 孢子初始速度 (文档标准)
constexpr float EJECT_COST_RATIO = 0.02f;          // 分数消耗比例
constexpr float EJECT_COOLDOWN = 0.1f;             // 吐孢子冷却时间
constexpr int EJECT_VEL_ZERO_FRAME = 20;           // 孢子速度衰减帧数 (GoBigger标准)
constexpr int EJECT_MIN_SCORE = 3200;              // 最小喷射分数 (GoBigger标准)

// 地图参数 (扩大地图以匹配更合理的视角)
constexpr int MAP_WIDTH = 4000;                    // 地图宽度 (扩大1.5倍)
constexpr int MAP_HEIGHT = 4000;                   // 地图高度 (扩大1.5倍)
constexpr int VIEWPORT_WIDTH = 1920;               // 视窗宽度
constexpr int VIEWPORT_HEIGHT = 1080;              // 视窗高度

// 食物参数 (大幅降低数量以确保流畅性能)
constexpr int FOOD_COUNT_INIT = 3000;               // 提升到3000，测试四叉树优化效果
constexpr int FOOD_COUNT_MAX = 4000;               // 提升到4000，测试四叉树优化效果
constexpr int FOOD_REFRESH_FRAMES = 12;            // 稍微放慢补充速度
constexpr float FOOD_REFRESH_PERCENT = 0.01f;      // 食物补充比例 (GoBigger标准: 1%)
constexpr int FOOD_SCORE = 100;                    // 普通食物分数 (GoBigger标准)
constexpr float FOOD_RADIUS = 5.0f;                // 食物半径 (文档标准)
constexpr float FOOD_VISUAL_SCALE = 3.0f;          // 食物视觉&碰撞半径缩放
constexpr int FOOD_MIN_SCORE = 100;                // 食物最小分数 (GoBigger标准)
constexpr int FOOD_MAX_SCORE = 100;                // 食物最大分数 (GoBigger标准)

// 荆棘参数 (参考GoBigger原版)
constexpr int THORNS_COUNT = 9;                    // 地图荆棘总数 (GoBigger标准)
constexpr int THORNS_COUNT_MAX = 12;               // 荆棘最大数量 (GoBigger标准)
constexpr int THORNS_REFRESH_FRAMES = 120;         // 荆棘补充间隔帧数 (GoBigger标准)
constexpr float THORNS_REFRESH_PERCENT = 0.2f;     // 荆棘补充比例 (GoBigger标准: 20%)
constexpr int THORNS_MIN_SCORE = 10000;            // 荆棘最小分数 (GoBigger标准)
constexpr int THORNS_MAX_SCORE = 15000;            // 荆棘最大分数 (GoBigger标准)
constexpr float THORNS_DAMAGE_RATIO = 0.2f;        // 荆棘伤害比例（降低到20%，更合理）
constexpr float THORNS_SPORE_SPEED = 10.0f;        // 荆棘吃孢子后的移动速度
constexpr int THORNS_SPORE_DECAY_FRAMES = 20;      // 荆棘速度衰减帧数
constexpr int THORNS_SPLIT_MAX_COUNT = 10;         // 荆棘分裂最大新球数量
constexpr int THORNS_SPLIT_MAX_SCORE = 5000;       // 荆棘分裂新球最大分数

// 孢子参数
constexpr int SPORE_LIFESPAN = 600;                // 孢子生命周期(帧)

// 衰减参数 (参考GoBigger原版)
constexpr float DECAY_START_SCORE = 2600.0f;       // 开始衰减的分数 (GoBigger标准)
constexpr float DECAY_RATE = 0.00005f;             // 衰减速率 (GoBigger标准)

// 合并参数
constexpr float RECOMBINE_RADIUS = 1.1f;           // 合并半径倍数（稍微重叠就合并）
constexpr int BIG_FOOD_SCORE = 500;                // 大食物分数
constexpr float BIG_FOOD_RADIUS = 50.0f;           // 大食物半径
constexpr float BIG_FOOD_SPAWN_RATE = 0.1f;        // 大食物生成概率

// 渲染参数
constexpr float ZOOM_MIN = 0.5f;                   // 最小缩放
constexpr float ZOOM_MAX = 2.0f;                   // 最大缩放
constexpr float GRID_SIZE = 100.0f;                // 网格大小
constexpr int NAME_FONT_SIZE = 16;                 // 名字字体大小
constexpr int SCORE_FONT_SIZE = 14;                // 分数字体大小

// ============ 工具函数 ============

// 分数到半径转换 (GoBigger原版公式: radius = sqrt(score / 100 * 0.042 + 0.15))
inline float scoreToRadius(float score) {
    return std::sqrt(score / 100.0f * 0.042f + 0.15f) * RADIUS_DISPLAY_SCALE;
}

// 半径到分数转换
inline float radiusToScore(float radius) {
    float scaledRadius = radius / RADIUS_DISPLAY_SCALE;
    return (scaledRadius * scaledRadius - 0.15f) / 0.042f * 100.0f;
}

// 计算移动速度 (基于分数计算)
inline float calculateSpeed(float score) {
    float radius = scoreToRadius(score);
    return BASE_SPEED / std::sqrt(score / static_cast<float>(CELL_MIN_SCORE));
}

// GoBigger风格的动态速度计算 (原版公式: (2.35 + 5.66 / radius) * ratio)
inline float calculateDynamicSpeed(float radius, float inputRatio = 1.0f) {
    return (SPEED_RADIUS_COEFF_A + SPEED_RADIUS_COEFF_B / radius) * inputRatio;
}

// 动态加速度计算 (基于GoBigger的acc_weight=30)
inline float calculateDynamicAcceleration(float radius, float inputRatio = 1.0f) {
    return 30.0f * inputRatio; // GoBigger标准加速度
}

// 检查是否可以吞噬 (GoBigger标准: 1.3倍比例)
inline bool canEat(float eaterScore, float targetScore) {
    return eaterScore / targetScore >= EAT_RATIO;
}

// 检查是否可以分裂 (GoBigger标准: score >= 3600)
inline bool canSplit(float score, int currentCellCount) {
    return score >= SPLIT_MIN_SCORE && currentCellCount < MAX_SPLIT_COUNT;
}

// 检查是否可以喷射孢子 (GoBigger标准: score >= 3200)
inline bool canEject(float score) {
    return score >= EJECT_MIN_SCORE;
}

// ============ 颜色定义 ============
inline QVector<QColor> getPlayerColors() {
    return {
        QColor(255, 0, 0),      // 红色
        QColor(0, 255, 0),      // 绿色
        QColor(0, 0, 255),      // 蓝色
        QColor(255, 255, 0),    // 黄色
        QColor(255, 0, 255),    // 紫色
        QColor(0, 255, 255),    // 青色
        QColor(255, 128, 0),    // 橙色
        QColor(128, 0, 255),    // 紫罗兰
    };
}

// 优化：使用固定的少量食物颜色，提升渲染性能
inline QVector<QColor> getFoodColors() {
    static const QVector<QColor> colors = {
        QColor(255, 100, 100),  // 浅红
        QColor(100, 255, 100),  // 浅绿
        QColor(100, 100, 255),  // 浅蓝
        QColor(255, 255, 100),  // 浅黄
    };
    return colors;
}

// 高性能：获取固定的食物颜色（避免每次都创建QVector）
inline const QColor& getStaticFoodColor(int index) {
    static const QColor foodColors[4] = {
        QColor(255, 100, 100),  // 浅红
        QColor(100, 255, 100),  // 浅绿
        QColor(100, 100, 255),  // 浅蓝
        QColor(255, 255, 100),  // 浅黄
    };
    return foodColors[index % 4];
}

} // namespace GoBiggerConfig

#endif // GOBIGGERCONFIG_H
