#ifndef GOBIGGERCONFIG_H
#define GOBIGGERCONFIG_H

#include <QColor>
#include <QVector>

// GoBigger标准配置参数
namespace GoBiggerConfig {

// ============ 核心数值参数 ============

// 细胞基础参数
constexpr float CELL_MIN_RADIUS = 22.0f;           // 最小半径（再次增大）
constexpr float CELL_MAX_RADIUS = 300.0f;          // 最大半径
constexpr int CELL_MIN_MASS = 50;                  // 初始质量（再次增大）
constexpr int CELL_MAX_MASS = 22500;               // 最大质量(半径300时)
constexpr float MASS_TO_RADIUS_RATIO = 1.4f;       // 质量到半径转换: radius = sqrt(mass) * 1.4（再次增大）

// 移动参数 (基于GoBigger动态计算)
constexpr float BASE_SPEED = 90.0f;               // 基础移动速度
constexpr float SPEED_DECAY_FACTOR = 1.0f;         // 速度衰减因子
constexpr float ACCELERATION_FACTOR = 1.6f;        // 加速度因子
constexpr float SPEED_RADIUS_COEFF_A = 2.35f;      // 速度计算系数A
constexpr float SPEED_RADIUS_COEFF_B = 5.66f;      // 速度计算系数B
// 实际速度 = BASE_SPEED / sqrt(mass / CELL_MIN_MASS)

// 分裂参数
constexpr int SPLIT_MIN_MASS = 36;                 // 最小分裂质量
constexpr int MAX_SPLIT_COUNT = 16;                // 最大分裂数量
constexpr float SPLIT_BOOST_SPEED = 350.0f;        // 分裂冲刺速度（降低一点）
constexpr float SPLIT_COOLDOWN = 1.0f;             // 分裂冷却时间(秒)
constexpr float MERGE_DELAY = 20.0f;               // 合并延迟时间(秒)

// 吞噬参数
constexpr float EAT_RATIO = 1.25f;                 // 吞噬质量比例
constexpr float EAT_DISTANCE_RATIO = 0.8f;         // 吞噬距离比例

// 吐孢子参数
constexpr float EJECT_MASS = 5.0f;                 // 孢子质量 (调整到5.0，更合理的最低质量)
constexpr float EJECT_SPEED = 200.0f;              // 孢子初始速度 (增大使动画更明显)
constexpr float EJECT_COST_RATIO = 0.02f;          // 质量消耗比例
constexpr float EJECT_COOLDOWN = 0.1f;             // 吐孢子冷却时间
constexpr int EJECT_VEL_ZERO_FRAME = 10;           // 孢子速度衰减帧数 (GoBigger标准)

// 地图参数
constexpr int MAP_WIDTH = 4000;                    // 地图宽度
constexpr int MAP_HEIGHT = 4000;                   // 地图高度
constexpr int VIEWPORT_WIDTH = 1920;               // 视窗宽度
constexpr int VIEWPORT_HEIGHT = 1080;              // 视窗高度

// 食物参数
constexpr int FOOD_COUNT = 800;                    // 地图食物总数
constexpr int FOOD_MASS = 6;                       // 普通食物质量（再次增大）
constexpr float FOOD_RADIUS = 8.0f;                // 食物半径（再次增大）
constexpr float FOOD_MIN_MASS = 4.0f;              // 食物最小质量（再次增大）
constexpr float FOOD_MAX_MASS = 8.0f;              // 食物最大质量（再次增大）

// 荆棘参数
constexpr int THORNS_COUNT = 40;                   // 地图荆棘总数
constexpr int THORNS_MIN_MASS = 50;                // 荆棘最小质量
constexpr int THORNS_MAX_MASS = 100;               // 荆棘最大质量
constexpr float THORNS_DAMAGE_RATIO = 0.8f;        // 荆棘伤害比例

// 孢子参数
constexpr int SPORE_LIFESPAN = 600;                // 孢子生命周期(帧)

// 衰减参数
constexpr float DECAY_START_MASS = 500.0f;         // 开始衰减的质量
constexpr float DECAY_RATE = 0.001f;               // 衰减速率

// 合并参数
constexpr float RECOMBINE_RADIUS = 3.0f;           // 合并半径倍数
constexpr int BIG_FOOD_MASS = 5;                   // 大食物质量
constexpr float BIG_FOOD_RADIUS = 8.0f;            // 大食物半径
constexpr float BIG_FOOD_SPAWN_RATE = 0.1f;        // 大食物生成概率

// 渲染参数
constexpr float ZOOM_MIN = 0.2f;                   // 最小缩放
constexpr float ZOOM_MAX = 2.0f;                   // 最大缩放
constexpr float GRID_SIZE = 100.0f;                // 网格大小
constexpr int NAME_FONT_SIZE = 16;                 // 名字字体大小
constexpr int MASS_FONT_SIZE = 14;                 // 质量字体大小

// ============ 工具函数 ============

// 质量到半径转换
inline float massToRadius(float mass) {
    return std::sqrt(mass * MASS_TO_RADIUS_RATIO);
}

// 半径到质量转换  
inline float radiusToMass(float radius) {
    return radius * radius / MASS_TO_RADIUS_RATIO;
}

// 计算移动速度 (基于GoBigger算法)
inline float calculateSpeed(float mass) {
    return BASE_SPEED / std::sqrt(mass / static_cast<float>(CELL_MIN_MASS));
}

// GoBigger风格的动态速度计算
inline float calculateDynamicSpeed(float radius, float inputRatio = 1.0f) {
    // GoBigger公式: (2.35 + 5.66 / radius) * ratio
    return (SPEED_RADIUS_COEFF_A + SPEED_RADIUS_COEFF_B / radius) * inputRatio * 10.0f; // 乘以10放大速度
}

// 动态加速度计算
inline float calculateDynamicAcceleration(float radius, float inputRatio = 1.0f) {
    // 基于半径的加速度，小球加速更快
    return ACCELERATION_FACTOR * (1.0f + 3.0f / radius) * inputRatio;
}

// 检查是否可以吞噬
inline bool canEat(float eaterMass, float targetMass) {
    return eaterMass / targetMass >= EAT_RATIO;
}

// 检查是否可以分裂
inline bool canSplit(float mass, int currentCellCount) {
    return mass >= SPLIT_MIN_MASS && currentCellCount < MAX_SPLIT_COUNT;
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

inline QVector<QColor> getFoodColors() {
    return {
        QColor(255, 100, 100),  // 浅红
        QColor(100, 255, 100),  // 浅绿
        QColor(100, 100, 255),  // 浅蓝
        QColor(255, 255, 100),  // 浅黄
        QColor(255, 100, 255),  // 浅品红
        QColor(100, 255, 255),  // 浅青
        QColor(255, 150, 100),  // 浅橙
        QColor(200, 150, 255),  // 浅紫
    };
}

} // namespace GoBiggerConfig

#endif // GOBIGGERCONFIG_H
