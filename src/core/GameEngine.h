#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QPointF>
#include <QVector2D>
#include <QRandomGenerator>
#include <memory>

// 包含数据类的完整定义
#include "data/BaseBallData.h"

// 前向声明
class CloneBallData;
class FoodBallData;
class SporeBallData;
class ThornsBallData;
class CoreQuadTree;

// AI接口数据结构
struct Action {
    float direction_x = 0.0f;
    float direction_y = 0.0f;
    int action_type = 0; // 0: 无动作, 1: 吐球, 2: 分裂
    
    Action() = default;
    Action(float dx, float dy, int type) : direction_x(dx), direction_y(dy), action_type(type) {}
};

struct GlobalState {
    QVector<int> border;           // [map_width, map_height] 
    int total_frame = 0;
    int last_frame_count = 0;
    QMap<int, float> leaderboard;  // 队伍ID -> 分数
};

struct PlayerState {
    QVector<float> rectangle;      // [x_min, y_min, x_max, y_max] - 玩家视野矩形
    
    // overlap结构：视野内的对象，每种类型的数量都有固定限制
    QVector<QVector<float>> food;  // 最多50个，每个: [x, y, radius, score]
    QVector<QVector<float>> thorns; // 最多20个，每个: [x, y, radius, score, vx, vy]
    QVector<QVector<float>> spore;  // 最多10个，每个: [x, y, radius, score, vx, vy]
    QVector<QVector<float>> clone;  // 最多30个，每个: [x, y, radius, score, vx, vy, dir_x, dir_y, team_id, player_id]
    
    float score = 0.0f;
    bool can_eject = false;
    bool can_split = false;
};

struct Observation {
    GlobalState global_state;
    QMap<int, PlayerState> player_states; // player_id -> PlayerState
};

class GameEngine : public QObject
{
    Q_OBJECT

public:
    struct Config {
        struct Border {
            qreal minx, maxx, miny, maxy;
            Border(qreal minX = -3000, qreal maxX = 3000, qreal minY = -3000, qreal maxY = 3000)
                : minx(minX), maxx(maxX), miny(minY), maxy(maxY) {}
        };
        
        Border gameBorder;
        int initFoodCount = 3000;
        int maxFoodCount = 4000;
        int initThornsCount = 9;
        int maxThornsCount = 12;
        int foodRefreshFrames = 12;
        int thornsRefreshFrames = 120;
        float foodRefreshPercent = 0.01f;
        float thornsRefreshPercent = 0.2f;
        int thornsScoreMin = 10000;
        int thornsScoreMax = 15000;
        qreal gameUpdateInterval = 16; // 60 FPS
        
        Config() = default;
    };

    explicit GameEngine(const Config& config = Config(), QObject* parent = nullptr);
    ~GameEngine();

    // 强化学习环境接口
    Observation reset();
    Observation step(const Action& action);
    bool isDone() const;
    Observation getObservation() const; // 移到public
    
    // 游戏管理接口
    void startGame();
    void pauseGame();
    void resetGame();
    bool isGameRunning() const { return m_gameRunning; }
    
    // 玩家管理
    CloneBallData* createPlayer(int teamId, int playerId, const QPointF& position = QPointF());
    CloneBallData* getPlayer(int teamId, int playerId) const;
    QVector<CloneBallData*> getPlayers() const { return m_players; }
    
    // 数据访问（供GUI使用）
    QVector<BaseBallData*> getAllBalls() const;
    QVector<FoodBallData*> getFoodBalls() const { return m_foodBalls; }
    QVector<SporeBallData*> getSporeBalls() const { return m_sporeBalls; }
    QVector<ThornsBallData*> getThornsBalls() const { return m_thornsBalls; }
    
    // 统计信息
    int getTotalFrames() const { return m_totalFrames; }
    float getTotalPlayerScore(int teamId, int playerId) const;

signals:
    void gameStarted();
    void gamePaused();
    void gameReset();
    void ballAdded(BaseBallData* ball);
    void ballRemoved(BaseBallData* ball);

private slots:
    void updateGame();
    void spawnFood();
    void spawnThorns();

private:
    Config m_config;
    bool m_gameRunning;
    int m_totalFrames;
    int m_nextBallId;
    int m_foodRefreshFrameCount;
    int m_thornsRefreshFrameCount;
    
    // 球体存储
    QMap<int, BaseBallData*> m_allBalls;
    QVector<CloneBallData*> m_players;
    QVector<FoodBallData*> m_foodBalls;
    QVector<SporeBallData*> m_sporeBalls;
    QVector<ThornsBallData*> m_thornsBalls;
    
    // 空间分区
    std::unique_ptr<CoreQuadTree> m_quadTree;
    
    // 内部管理方法
    void addBall(BaseBallData* ball);
    void removeBall(BaseBallData* ball);
    void clearAllBalls();
    int getNextBallId() { return m_nextBallId++; }
    
    // 位置生成
    QPointF generateRandomPosition() const;
    QPointF generateRandomFoodPosition() const;
    QPointF generateRandomThornsPosition() const;
    
    // 碰撞检测
    void checkCollisions();
    void checkCollisionsBetween(BaseBallData* ball1, BaseBallData* ball2);
    void checkPlayerBallsMerging(int teamId, int playerId);
    QVector<CloneBallData*> getPlayerBalls(int teamId, int playerId) const;
    
    // AI接口实现
    PlayerState getPlayerState(int playerId) const;
    QVector<QVector<float>> getObjectsInView(const QRectF& viewRect, BaseBallData::BallType type) const;
    QRectF calculatePlayerViewRect(int playerId) const;
    
    // 数据预处理
    QVector<QVector<float>> preprocessObjects(const QVector<QVector<float>>& objects, int maxCount) const;
    QVector<QVector<float>> sortObjectsByDistance(const QVector<QVector<float>>& objects, const QPointF& playerCenter) const;
};

#endif // GAMEENGINE_H
