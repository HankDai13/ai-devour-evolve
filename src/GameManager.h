#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QHash>
#include <QGraphicsScene>
#include <QRandomGenerator>
#include "BaseBall.h"
#include "GoBiggerConfig.h"
#include "QuadTree.h"

// Forward declarations
class CloneBall;
class FoodBall;
class SporeBall;
class ThornsBall;
namespace GoBigger { 
    namespace AI { 
        class SimpleAIPlayer;
        // AI策略枚举前置声明
        enum class AIStrategy {
            RANDOM,      // 随机移动
            FOOD_HUNTER, // 寻找食物
            AGGRESSIVE,  // 攻击性策略
            MODEL_BASED  // 基于模型的策略
        };
    } 
}

class GameManager : public QObject
{
    Q_OBJECT

public:
    struct Config {
        // 游戏区域
        Border gameBorder = Border(-400, 400, -400, 400);
        
        // 食物配置 (完全对齐GoBigger原版策略)
        int maxFoodCount = GoBiggerConfig::FOOD_COUNT_MAX;      // 最大食物数量 (900)
        int initFoodCount = GoBiggerConfig::FOOD_COUNT_INIT;    // 初始食物数量 (800)
        int foodRefreshFrames = GoBiggerConfig::FOOD_REFRESH_FRAMES;  // 补充间隔帧数 (8帧)
        float foodRefreshPercent = GoBiggerConfig::FOOD_REFRESH_PERCENT; // 补充比例 (1%)
        qreal foodScoreMin = GoBiggerConfig::FOOD_MIN_SCORE;    // 食物最小分数
        qreal foodScoreMax = GoBiggerConfig::FOOD_MAX_SCORE;    // 食物最大分数
        
        // 荆棘配置 (GoBigger标准)
        int initThornsCount = GoBiggerConfig::THORNS_COUNT;     // 初始荆棘数量 (9)
        int maxThornsCount = GoBiggerConfig::THORNS_COUNT_MAX;  // 最大荆棘数量 (12)
        int thornsRefreshFrames = GoBiggerConfig::THORNS_REFRESH_FRAMES; // 补充间隔帧数 (120帧)
        float thornsRefreshPercent = GoBiggerConfig::THORNS_REFRESH_PERCENT; // 补充比例 (20%)
        qreal thornsScoreMin = GoBiggerConfig::THORNS_MIN_SCORE; // 荆棘最小分数
        qreal thornsScoreMax = GoBiggerConfig::THORNS_MAX_SCORE; // 荆棘最大分数
        
        // 玩家配置
        qreal playerScoreInit = 10.0;   // 玩家初始分数
        qreal playerSplitScoreMin = 16.0; // 分裂最小分数
        qreal playerEjectScoreMin = 6.0;  // 孢子喷射最小分数
        
        // 游戏更新频率
        int gameUpdateInterval = 16;    // 60 FPS
        
        // 碰撞检测配置
        qreal collisionCheckRadius = 50.0;
        qreal eatRatioThreshold = 1.15; // 吃掉其他球的大小比例阈值
        
        Config() = default;
    };

    explicit GameManager(QGraphicsScene* scene, const Config& config = Config(), QObject* parent = nullptr);
    ~GameManager();

    // 游戏控制
    void startGame();
    void pauseGame();
    void resetGame();
    bool isGameRunning() const { return m_gameRunning; }

    // 玩家管理
    CloneBall* createPlayer(int teamId, int playerId, const QPointF& position = QPointF());
    void removePlayer(CloneBall* player);
    CloneBall* getPlayer(int teamId, int playerId) const;
    QVector<CloneBall*> getPlayers() const { return m_players; }
    
    // 同玩家分身球合并检查 - 新增方法
    void checkPlayerBallsMerging(int teamId, int playerId);
    QVector<CloneBall*> getPlayerBalls(int teamId, int playerId) const;

    // 球管理
    void addBall(BaseBall* ball);
    void removeBall(BaseBall* ball);
    QVector<BaseBall*> getAllBalls() const;
    QVector<BaseBall*> getBallsNear(const QPointF& position, qreal radius) const;
    
    // 视野优化 - 只获取指定区域内的球
    QVector<BaseBall*> getBallsInRect(const QRectF& rect) const;
    QVector<FoodBall*> getFoodBallsInRect(const QRectF& rect) const;

    // 游戏状态
    const Config& config() const { return m_config; }
    int getCurrentBallId() const { return m_nextBallId; }
    
    // AI玩家管理
    bool addAIPlayer(int teamId, int playerId, const QString& aiModelPath = "");
    // 新增：支持指定AI策略的方法
    bool addAIPlayerWithStrategy(int teamId, int playerId, 
                                 GoBigger::AI::AIStrategy strategy,
                                 const QString& aiModelPath = "");
    void removeAIPlayer(int teamId, int playerId);
    void startAllAI();
    void stopAllAI();
    void removeAllAI();
    
    // AI玩家访问方法
    QVector<GoBigger::AI::SimpleAIPlayer*> getAIPlayers() const { return m_aiPlayers; }
    
    // 统计信息
    int getFoodCount() const { return m_foodBalls.size(); }
    int getThornsCount() const { return m_thornsBalls.size(); }
    int getPlayerCount() const { return m_players.size(); }

signals:
    void gameStarted();
    void gamePaused();
    void gameReset();
    void playerAdded(CloneBall* player);
    void playerRemoved(CloneBall* player);
    void ballAdded(BaseBall* ball);
    void ballRemoved(BaseBall* ball);

private slots:
    void updateGame();
    void spawnFood();
    void spawnThorns();
    void handleBallRemoved(BaseBall* ball);
    void handlePlayerSplit(CloneBall* originalBall, const QVector<CloneBall*>& newBalls);
    void handleSporeEjected(CloneBall* ball, SporeBall* spore);
    void handleThornsCollision(ThornsBall* thorns, CloneBall* ball);
    void handleThornsEaten(CloneBall* ball, ThornsBall* thorns); // 新增：处理吃荆棘球
    void handleAIPlayerDestroyed(GoBigger::AI::SimpleAIPlayer* aiPlayer); // 新增：处理AI玩家被销毁

private:
    QGraphicsScene* m_scene;
    Config m_config;
    bool m_gameRunning;
    
    // 定时器
    QTimer* m_gameTimer;
    QTimer* m_foodTimer;
    QTimer* m_thornsTimer;
    
    // 球的管理
    QVector<CloneBall*> m_players;
    QVector<FoodBall*> m_foodBalls;
    QVector<SporeBall*> m_sporeBalls;
    QVector<ThornsBall*> m_thornsBalls;
    QHash<int, BaseBall*> m_allBalls;
    
    // AI玩家管理
    QVector<GoBigger::AI::SimpleAIPlayer*> m_aiPlayers;
    QString m_defaultAIModelPath;
    
    int m_nextBallId;
    
    // GoBigger风格的食物刷新机制
    int m_foodRefreshFrameCount;
    
    // GoBigger风格的荆棘刷新机制
    int m_thornsRefreshFrameCount;
    
    // 空间分区优化 - 四叉树
    std::unique_ptr<QuadTree> m_quadTree;
    
    // 初始化
    void initializeTimers();
    void connectBallSignals(BaseBall* ball);
    void disconnectBallSignals(BaseBall* ball);
    
    // 生成函数
    QPointF generateRandomPosition() const;
    QPointF generateRandomFoodPosition() const;
    QPointF generateRandomThornsPosition() const;
    
    // 碰撞检测 - GoBigger优化版本
    void checkCollisions();
    void checkCollisionsOptimized();
    void checkCollisionsBetween(BaseBall* ball1, BaseBall* ball2);
    QVector<BaseBall*> getMovingBalls() const;
    void optimizeSporeCollisions();
    
    // 清理函数
    void clearAllBalls();
    void removeFromScene(BaseBall* ball);
    
    // ID管理
    int getNextBallId() { return m_nextBallId++; }
};

#endif // GAMEMANAGER_H
