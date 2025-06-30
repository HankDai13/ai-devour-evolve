#include "MultiPlayerManager.h"
#include "AIPlayer.h"
#include <QDebug>
#include <QFileInfo>
#include <algorithm>

namespace GoBigger {
namespace Multiplayer {

MultiPlayerManager::MultiPlayerManager(GameManager* gameManager, QObject* parent)
    : QObject(parent)
    , m_gameManager(gameManager)
    , m_gameRunning(false)
    , m_gamePaused(false)
    , m_maxPlayers(8)
{
    if (m_gameManager) {
        connect(m_gameManager, &GameManager::gameStarted, this, &MultiPlayerManager::onGameManagerStarted);
        connect(m_gameManager, &GameManager::gamePaused, this, &MultiPlayerManager::onGameManagerPaused);
    }
    
    qDebug() << "MultiPlayerManager initialized with max players:" << m_maxPlayers;
}

MultiPlayerManager::~MultiPlayerManager()
{
    stopMultiPlayerGame();
}

bool MultiPlayerManager::addPlayer(const PlayerInfo& playerInfo)
{
    // 检查玩家数量限制
    if (m_players.size() >= m_maxPlayers) {
        qWarning() << "Cannot add player: maximum player count reached (" << m_maxPlayers << ")";
        return false;
    }
    
    // 检查玩家是否已存在
    if (isPlayerExists(playerInfo.teamId, playerInfo.playerId)) {
        qWarning() << "Player already exists: team" << playerInfo.teamId << "player" << playerInfo.playerId;
        return false;
    }
    
    // 添加玩家信息
    PlayerInfo newPlayer = playerInfo;
    
    // 根据玩家类型进行初始化
    if (newPlayer.type == PlayerType::AI) {
        // AI玩家需要模型路径
        QString modelPath = newPlayer.aiModelPath.isEmpty() ? m_defaultAIModelPath : newPlayer.aiModelPath;
        if (modelPath.isEmpty()) {
            qWarning() << "No AI model path specified for AI player";
            return false;
        }
        
        // 检查模型文件是否存在
        QFileInfo modelFile(modelPath);
        if (!modelFile.exists()) {
            qWarning() << "AI model file does not exist:" << modelPath;
            return false;
        }
        
        newPlayer.aiModelPath = modelPath;
        
        // 如果游戏已运行，立即添加AI玩家
        if (m_gameRunning && m_gameManager) {
            if (!m_gameManager->addAIPlayer(newPlayer.teamId, newPlayer.playerId, modelPath)) {
                qWarning() << "Failed to add AI player to GameManager";
                return false;
            }
        }
    } else {
        // 人类玩家的处理逻辑（暂时为空，可以后续扩展）
        if (m_gameRunning && m_gameManager) {
            // TODO: 添加人类玩家到GameManager
            qDebug() << "Human player addition during running game not yet implemented";
        }
    }
    
    newPlayer.active = true;
    m_players.append(newPlayer);
    
    qDebug() << "Successfully added" << (newPlayer.type == PlayerType::AI ? "AI" : "Human") 
             << "player: team" << newPlayer.teamId << "player" << newPlayer.playerId 
             << "name:" << newPlayer.name;
    
    updatePlayerCounts();
    emit playerAdded(newPlayer);
    
    return true;
}

bool MultiPlayerManager::removePlayer(int teamId, int playerId)
{
    int index = findPlayerIndex(teamId, playerId);
    if (index < 0) {
        qWarning() << "Player not found: team" << teamId << "player" << playerId;
        return false;
    }
    
    PlayerInfo& player = m_players[index];
    
    // 如果游戏正在运行，从GameManager中移除
    if (m_gameRunning && m_gameManager) {
        if (player.type == PlayerType::AI) {
            m_gameManager->removeAIPlayer(teamId, playerId);
        } else {
            // TODO: 移除人类玩家
        }
    }
    
    qDebug() << "Removing" << (player.type == PlayerType::AI ? "AI" : "Human")
             << "player: team" << teamId << "player" << playerId;
    
    m_players.removeAt(index);
    
    updatePlayerCounts();
    emit playerRemoved(teamId, playerId);
    
    return true;
}

void MultiPlayerManager::removeAllPlayers()
{
    if (m_gameRunning && m_gameManager) {
        m_gameManager->stopAllAI();
    }
    
    int removedCount = m_players.size();
    m_players.clear();
    
    qDebug() << "Removed all" << removedCount << "players";
    updatePlayerCounts();
}

void MultiPlayerManager::startMultiPlayerGame()
{
    if (m_gameRunning) {
        qWarning() << "Game is already running";
        return;
    }
    
    if (m_players.isEmpty()) {
        qWarning() << "No players to start the game";
        return;
    }
    
    if (!m_gameManager) {
        qWarning() << "GameManager not available";
        return;
    }
    
    qDebug() << "Starting multiplayer game with" << m_players.size() << "players";
    
    // 首先启动游戏引擎
    m_gameManager->startGame();
    
    // 添加所有AI玩家到游戏中
    for (const auto& player : m_players) {
        if (player.type == PlayerType::AI && player.active) {
            if (!m_gameManager->addAIPlayer(player.teamId, player.playerId, player.aiModelPath)) {
                qWarning() << "Failed to add AI player" << player.teamId << player.playerId;
            }
        }
    }
    
    // 启动所有AI
    m_gameManager->startAllAI();
    
    m_gameRunning = true;
    m_gamePaused = false;
    
    qDebug() << "Multiplayer game started successfully";
    emit gameStarted();
}

void MultiPlayerManager::stopMultiPlayerGame()
{
    if (!m_gameRunning) {
        return;
    }
    
    qDebug() << "Stopping multiplayer game";
    
    if (m_gameManager) {
        m_gameManager->stopAllAI();
        m_gameManager->pauseGame();
    }
    
    m_gameRunning = false;
    m_gamePaused = false;
    
    qDebug() << "Multiplayer game stopped";
    emit gameStopped();
}

void MultiPlayerManager::pauseMultiPlayerGame()
{
    if (!m_gameRunning || m_gamePaused) {
        return;
    }
    
    qDebug() << "Pausing multiplayer game";
    
    if (m_gameManager) {
        m_gameManager->stopAllAI();
        m_gameManager->pauseGame();
    }
    
    m_gamePaused = true;
    
    qDebug() << "Multiplayer game paused";
    emit gamePaused();
}

void MultiPlayerManager::resumeMultiPlayerGame()
{
    if (!m_gameRunning || !m_gamePaused) {
        return;
    }
    
    qDebug() << "Resuming multiplayer game";
    
    if (m_gameManager) {
        m_gameManager->startGame();
        m_gameManager->startAllAI();
    }
    
    m_gamePaused = false;
    
    qDebug() << "Multiplayer game resumed";
    emit gameResumed();
}

QVector<PlayerInfo> MultiPlayerManager::getActivePlayers() const
{
    QVector<PlayerInfo> activePlayers;
    for (const auto& player : m_players) {
        if (player.active) {
            activePlayers.append(player);
        }
    }
    return activePlayers;
}

PlayerInfo MultiPlayerManager::getPlayer(int teamId, int playerId) const
{
    int index = findPlayerIndex(teamId, playerId);
    if (index >= 0) {
        return m_players[index];
    }
    return PlayerInfo(); // 返回默认构造的PlayerInfo
}

int MultiPlayerManager::getAIPlayerCount() const
{
    return std::count_if(m_players.begin(), m_players.end(), 
                        [](const PlayerInfo& p) { return p.type == PlayerType::AI && p.active; });
}

int MultiPlayerManager::getHumanPlayerCount() const
{
    return std::count_if(m_players.begin(), m_players.end(), 
                        [](const PlayerInfo& p) { return p.type == PlayerType::HUMAN && p.active; });
}

void MultiPlayerManager::setDefaultAIModel(const QString& modelPath)
{
    QFileInfo fileInfo(modelPath);
    if (!fileInfo.exists()) {
        qWarning() << "Default AI model file does not exist:" << modelPath;
        return;
    }
    
    m_defaultAIModelPath = modelPath;
    qDebug() << "Default AI model set to:" << modelPath;
}

void MultiPlayerManager::onGameManagerStarted()
{
    qDebug() << "GameManager started notification received";
}

void MultiPlayerManager::onGameManagerPaused()
{
    qDebug() << "GameManager paused notification received";
}

bool MultiPlayerManager::isPlayerExists(int teamId, int playerId) const
{
    return findPlayerIndex(teamId, playerId) >= 0;
}

int MultiPlayerManager::findPlayerIndex(int teamId, int playerId) const
{
    for (int i = 0; i < m_players.size(); ++i) {
        const auto& player = m_players[i];
        if (player.teamId == teamId && player.playerId == playerId) {
            return i;
        }
    }
    return -1;
}

void MultiPlayerManager::updatePlayerCounts()
{
    int total = m_players.size();
    int ai = getAIPlayerCount();
    int human = getHumanPlayerCount();
    
    emit playerCountChanged(total, ai, human);
}

// GameModeHelper 实现
QVector<PlayerInfo> GameModeHelper::createAIvsAIMode(int aiCount, const QString& aiModelPath)
{
    QVector<PlayerInfo> players;
    
    for (int i = 0; i < aiCount; ++i) {
        PlayerInfo player(i, 0, PlayerType::AI, QString("AI_%1").arg(i), aiModelPath);
        players.append(player);
    }
    
    qDebug() << "Created AI vs AI mode with" << aiCount << "AI players";
    return players;
}

QVector<PlayerInfo> GameModeHelper::createHumanvsAIMode(int humanCount, int aiCount, const QString& aiModelPath)
{
    QVector<PlayerInfo> players;
    
    // 添加人类玩家
    for (int i = 0; i < humanCount; ++i) {
        PlayerInfo player(i, 0, PlayerType::HUMAN, QString("Human_%1").arg(i));
        players.append(player);
    }
    
    // 添加AI玩家
    for (int i = 0; i < aiCount; ++i) {
        PlayerInfo player(humanCount + i, 0, PlayerType::AI, QString("AI_%1").arg(i), aiModelPath);
        players.append(player);
    }
    
    qDebug() << "Created Human vs AI mode with" << humanCount << "humans and" << aiCount << "AIs";
    return players;
}

QVector<PlayerInfo> GameModeHelper::createMultiAIBattleMode(const QVector<QString>& aiModelPaths)
{
    QVector<PlayerInfo> players;
    
    for (int i = 0; i < aiModelPaths.size(); ++i) {
        PlayerInfo player(i, 0, PlayerType::AI, QString("AI_%1").arg(i), aiModelPaths[i]);
        players.append(player);
    }
    
    qDebug() << "Created Multi-AI battle mode with" << aiModelPaths.size() << "different AI models";
    return players;
}

QVector<PlayerInfo> GameModeHelper::createTeamBattleMode(int teamsCount, int playersPerTeam, 
                                                        PlayerType playerType, const QString& aiModelPath)
{
    QVector<PlayerInfo> players;
    
    for (int team = 0; team < teamsCount; ++team) {
        for (int player = 0; player < playersPerTeam; ++player) {
            QString playerName = QString("%1_Team%2_Player%3")
                               .arg(playerType == PlayerType::AI ? "AI" : "Human")
                               .arg(team)
                               .arg(player);
            
            PlayerInfo playerInfo(team, player, playerType, playerName, 
                                 playerType == PlayerType::AI ? aiModelPath : "");
            players.append(playerInfo);
        }
    }
    
    qDebug() << "Created Team battle mode with" << teamsCount << "teams," << playersPerTeam 
             << "players per team, player type:" << (playerType == PlayerType::AI ? "AI" : "Human");
    return players;
}

} // namespace Multiplayer
} // namespace GoBigger
