#ifndef MULTI_AGENT_GAME_ENGINE_H
#define MULTI_AGENT_GAME_ENGINE_H

// 先包含pybind11避免宏冲突
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <QObject>
#include <QGraphicsScene>
#include <QPointF>
#include <QVector2D>
#include <QRandomGenerator>

#include "../src_new/GameManager.h"
#include "../src_new/MultiPlayerManager.h"
#include "../src_new/SimpleAIPlayer.h"
#include "../src_new/CloneBall.h"
#include "../src_new/FoodBall.h"
#include "../src_new/ThornsBall.h"

namespace py = pybind11;

/**
 * 多智能体游戏引擎
 * 
 * 这个类将src_new中的多智能体系统包装成一个适合强化学习的环境。
 * 支持一个RL智能体与多个传统AI策略机器人对战，
 * 并提供团队排名作为奖励信号。
 */
class MultiAgentGameEngine : public QObject
{
    Q_OBJECT

public:
    struct Config {
        int maxFoodCount = 3000;          // 最大食物数量
        int initFoodCount = 2500;         // 初始食物数量
        int maxThornsCount = 12;          // 最大荆棘数量
        int initThornsCount = 9;          // 初始荆棘数量
        int gameUpdateInterval = 16;      // 游戏更新间隔(ms)
        int maxFrames = 3000;             // 最大游戏帧数
        int aiOpponentCount = 3;          // AI对手数量
        
        Config() = default;
    };

    explicit MultiAgentGameEngine(const Config& config = Config(), QObject* parent = nullptr);
    ~MultiAgentGameEngine();

    // ============ 强化学习环境接口 ============
    
    /**
     * 重置环境
     * @return 初始观察
     */
    py::dict reset();
    
    /**
     * 执行一步动作
     * @param actions 动作字典，包含 "rl_agent" 键对应RL智能体的动作
     * @return 新的观察
     */
    py::dict step(const py::dict& actions);
    
    /**
     * 检查游戏是否结束
     * @return 是否结束
     */
    bool isDone() const;
    
    /**
     * 获取当前观察
     * @return 观察字典
     */
    py::dict getObservation() const;
    
    /**
     * 获取奖励相关信息
     * @return 奖励信息字典
     */
    py::dict getRewardInfo() const;

    // ============ 配置和状态 ============
    
    const Config& getConfig() const { return m_config; }
    bool isGameRunning() const { return m_gameRunning; }
    int getFrameCount() const { return m_frameCount; }
    
    // 获取底层管理器（用于高级操作）
    GameManager* getGameManager() const { return m_gameManager; }
    GoBigger::Multiplayer::MultiPlayerManager* getMultiPlayerManager() const { return m_multiPlayerManager; }

private:
    // ============ 配置和核心组件 ============
    Config m_config;
    QGraphicsScene* m_scene;
    GameManager* m_gameManager;
    GoBigger::Multiplayer::MultiPlayerManager* m_multiPlayerManager;
    
    // ============ 游戏状态 ============
    int m_frameCount;
    bool m_gameRunning;
    int m_rlPlayerTeamId;      // RL智能体的团队ID
    int m_rlPlayerPlayerId;    // RL智能体的玩家ID
    
    // ============ 内部方法 ============
    
    /**
     * 设置传统AI对手
     */
    void setupTraditionalAIOpponents();
    
    /**
     * 执行RL智能体的动作
     * @param action Python列表格式的动作 [dx, dy, action_type]
     */
    void executeRLAction(const py::list& action);
    
    /**
     * 提取玩家的观察信息
     * @param player 玩家球体
     * @return 观察字典
     */
    py::dict extractPlayerObservation(CloneBall* player) const;
    
    /**
     * 计算团队排名
     * @return 排名列表，每个元素包含team_id, score, rank
     */
    py::list calculateTeamRanking() const;
    
    /**
     * 生成随机出生位置
     * @return 出生位置
     */
    QPointF generateRandomSpawnPosition() const;
};

#endif // MULTI_AGENT_GAME_ENGINE_H
