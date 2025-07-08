#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QGraphicsView>
#include <QKeyEvent>
#include <QTimer>
#include <QSet>
#include <QVector2D>
#include <QMap>

class GameManager;
class CloneBall;
class BaseBall;
class AIDebugWidget;
namespace GoBigger { namespace AI { class SimpleAIPlayer; } }

// GameView继承自QGraphicsView，成为游戏视图
class GameView : public QGraphicsView
{
    Q_OBJECT

public:
    GameView(QWidget *parent = nullptr);
    ~GameView();

    // 游戏控制
    void startGame();
    void pauseGame();
    void resetGame();
    bool isGameRunning() const;

    // 玩家控制
    CloneBall* getMainPlayer() const { return m_mainPlayer; }
    float getTotalPlayerScore() const; // 获取玩家总分数（所有分身球的分数总和）
    
    // 游戏管理器访问
    GameManager* getGameManager() const { return m_gameManager; }
    
    // AI玩家控制
    void addAIPlayer();
    void addRLAIPlayer();
    void addAIPlayerWithDialog();  // 新增：通过对话框添加AI玩家
    void startAllAI();
    void stopAllAI();
    void removeAllAI();
    
    // AI调试功能
    void showAIDebugConsole();
    void toggleAIDebugConsole();

protected:
    // 事件处理
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override; // 🔥 添加paintEvent以绘制UI层
    
    // 渲染优化
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private slots:
    void updateGameView();
    void onGameStarted();
    void onGamePaused();
    void onGameReset();
    void onPlayerAdded(CloneBall* player);
    void onPlayerRemoved(CloneBall* player);
    void onAIPlayerDestroyed(GoBigger::AI::SimpleAIPlayer* aiPlayer); // 新增：处理AI玩家销毁
    void onGameOver(int winningTeamId);

private:
    void showGameOverScreen(int winningTeamId);
    GameManager* m_gameManager;
    CloneBall* m_mainPlayer;
    
    // 输入处理
    QSet<int> m_pressedKeys;
    QTimer* m_inputTimer;
    
    // 视图控制
    qreal m_zoomFactor;
    bool m_followPlayer;
    
    // 智能缩放参数
    qreal m_targetZoom;
    qreal m_minVisionRadius;
    qreal m_maxVisionRadius;
    qreal m_scaleUpRatio;
    
    // 🔥 视角稳定性控制
    qreal m_lastTargetZoom;           // 上次的目标缩放
    QPointF m_lastCentroid;           // 上次的质心位置
    qreal m_zoomDeadZone;             // 缩放死区阈值
    qreal m_centroidDeadZone;         // 质心移动死区阈值
    int m_stableFrameCount;           // 稳定帧计数
    int m_requiredStableFrames;       // 需要的稳定帧数
    bool m_isInitialStabilizing;      // 是否在初始稳定阶段
    
    // AI控制
    QVector<GoBigger::AI::SimpleAIPlayer*> m_aiPlayers;
    
    // AI调试
    AIDebugWidget* m_aiDebugWidget;

    // 初始化
    void initializeView();
    void initializePlayer();
    void setupConnections();
    
    // 输入处理
    void processInput();
    QVector2D calculateMoveDirection() const;
    QVector2D calculateMouseDirection() const;  // 新增：鼠标方向计算
    
    // GoBigger风格玩家球管理
    QVector<CloneBall*> getAllPlayerBalls() const;
    QPointF calculatePlayerCentroidAll(const QVector<CloneBall*>& balls) const;
    
    // 视图更新
    void updateCamera();
    void adjustZoom();
    void calculateIntelligentZoom();
    void calculateIntelligentZoomGoBigger(const QVector<CloneBall*>& allPlayerBalls);
    qreal calculatePlayerRadius() const;
    QPointF calculatePlayerCentroid() const;
    
    // 视野裁剪优化
    QRectF getVisibleWorldRect() const;
    void updateVisibleItems();
    
    // 玩家操作
    void handleSplitAction();
    void handleEjectAction();

    // 队伍管理
    int assignTeamForNewAI(); // 为新AI分配队伍
    void updateTeamScores(); // 更新队伍分数
    QColor getTeamColor(int teamId) const; // 新增：获取队伍颜色

    // 队伍积分和排行榜
    QMap<int, float> calculateTeamScores() const;
    void drawTeamLeaderboard(QPainter* painter);
};

#endif // GAMEVIEW_H