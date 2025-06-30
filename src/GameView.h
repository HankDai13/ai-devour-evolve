#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QGraphicsView>
#include <QKeyEvent>
#include <QTimer>
#include <QSet>
#include <QVector2D>

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
    
    // AI玩家控制
    void addAIPlayer();
    void addRLAIPlayer();
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
    
    // 渲染优化
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private slots:
    void updateGameView();
    void onGameStarted();
    void onGamePaused();
    void onGameReset();
    void onPlayerAdded(CloneBall* player);
    void onPlayerRemoved(CloneBall* player);

private:
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
};

#endif // GAMEVIEW_H