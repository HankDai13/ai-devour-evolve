#pragma once
#include <QObject>
#include <QVector>
#include <QTimer>
#include <QString>
#include <memory>
#include "GameManager.h"

namespace GoBigger {
namespace Multiplayer {

// 玩家类型
enum class PlayerType {
    HUMAN,
    AI
};

// 玩家信息
struct PlayerInfo {
    int teamId;
    int playerId;
    PlayerType type;
    QString name;
    QString aiModelPath; // 仅AI玩家使用
    bool active;
    
    PlayerInfo(int team = 0, int player = 0, PlayerType t = PlayerType::HUMAN, 
               const QString& n = "", const QString& aiPath = "")
        : teamId(team), playerId(player), type(t), name(n), aiModelPath(aiPath), active(false) {}
};

// 多玩家游戏管理器
class MultiPlayerManager : public QObject {
    Q_OBJECT
    
public:
    explicit MultiPlayerManager(GameManager* gameManager, QObject* parent = nullptr);
    ~MultiPlayerManager();
    
    // 玩家管理
    bool addPlayer(const PlayerInfo& playerInfo);
    bool removePlayer(int teamId, int playerId);
    void removeAllPlayers();
    
    // 游戏控制
    void startMultiPlayerGame();
    void stopMultiPlayerGame();
    void pauseMultiPlayerGame();
    void resumeMultiPlayerGame();
    
    // 玩家信息
    QVector<PlayerInfo> getAllPlayers() const { return m_players; }
    QVector<PlayerInfo> getActivePlayers() const;
    PlayerInfo getPlayer(int teamId, int playerId) const;
    int getPlayerCount() const { return m_players.size(); }
    int getAIPlayerCount() const;
    int getHumanPlayerCount() const;
    
    // AI配置
    void setDefaultAIModel(const QString& modelPath);
    QString getDefaultAIModel() const { return m_defaultAIModelPath; }
    
    // 游戏状态
    bool isGameRunning() const { return m_gameRunning; }
    bool isGamePaused() const { return m_gamePaused; }
    
    // 配置
    void setMaxPlayers(int maxPlayers) { m_maxPlayers = maxPlayers; }
    int getMaxPlayers() const { return m_maxPlayers; }

signals:
    void playerAdded(const PlayerInfo& player);
    void playerRemoved(int teamId, int playerId);
    void gameStarted();
    void gameStopped();
    void gamePaused();
    void gameResumed();
    void playerCountChanged(int total, int ai, int human);

private slots:
    void onGameManagerStarted();
    void onGameManagerPaused();

private:
    GameManager* m_gameManager;
    QVector<PlayerInfo> m_players;
    QString m_defaultAIModelPath;
    bool m_gameRunning;
    bool m_gamePaused;
    int m_maxPlayers;
    
    // 内部方法
    bool isPlayerExists(int teamId, int playerId) const;
    int findPlayerIndex(int teamId, int playerId) const;
    void updatePlayerCounts();
};

// 快速设置常见游戏模式的工具类
class GameModeHelper {
public:
    // 创建AI vs AI模式
    static QVector<PlayerInfo> createAIvsAIMode(int aiCount, const QString& aiModelPath);
    
    // 创建Human vs AI模式
    static QVector<PlayerInfo> createHumanvsAIMode(int humanCount, int aiCount, const QString& aiModelPath);
    
    // 创建多AI对战模式
    static QVector<PlayerInfo> createMultiAIBattleMode(const QVector<QString>& aiModelPaths);
    
    // 创建团队对战模式
    static QVector<PlayerInfo> createTeamBattleMode(int teamsCount, int playersPerTeam, 
                                                    PlayerType playerType, const QString& aiModelPath = "");
};

} // namespace Multiplayer
} // namespace GoBigger
