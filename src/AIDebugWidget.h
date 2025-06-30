#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>
#include <QTimer>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <memory>
#include "SimpleAIPlayer.h"

class GameManager;

class AIDebugWidget : public QWidget {
    Q_OBJECT

public:
    explicit AIDebugWidget(GameManager* gameManager, QWidget* parent = nullptr);
    ~AIDebugWidget();

    // 更新AI状态显示
    void updateAIStatus();
    
    // 添加/移除AI监控
    void addAIPlayer(GoBigger::AI::SimpleAIPlayer* aiPlayer);
    void removeAIPlayer(GoBigger::AI::SimpleAIPlayer* aiPlayer);
    void clearAllAI();

    // 显示/隐藏调试窗口
    void showDebugInfo(bool show);

public slots:
    void onAIActionExecuted(const GoBigger::AI::AIAction& action);
    void onAIStrategyChanged(GoBigger::AI::SimpleAIPlayer::AIStrategy strategy);
    void refreshDebugInfo();

private slots:
    void onRefreshTimer();
    void onAIPlayerSelected();

private:
    void setupUI();
    void setupAIStatusPanel();
    void setupPerformancePanel();
    void setupDecisionPanel();
    void setupLogPanel();
    
    // UI组件
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_topLayout;
    QHBoxLayout* m_bottomLayout;
    
    // AI状态面板
    QGroupBox* m_aiStatusGroup;
    QListWidget* m_aiPlayersList;
    QLabel* m_aiCountLabel;
    QLabel* m_activeAILabel;
    
    // 性能监控面板
    QGroupBox* m_performanceGroup;
    QProgressBar* m_fpsBar;
    QProgressBar* m_cpuUsageBar;
    QProgressBar* m_memoryUsageBar;
    QLabel* m_fpsLabel;
    QLabel* m_cpuLabel;
    QLabel* m_memoryLabel;
    
    // AI决策信息面板
    QGroupBox* m_decisionGroup;
    QTableWidget* m_decisionTable;
    QLabel* m_selectedAILabel;
    QLabel* m_currentStrategyLabel;
    QLabel* m_lastActionLabel;
    
    // 日志面板
    QGroupBox* m_logGroup;
    QTextEdit* m_logTextEdit;
    
    // 数据和状态
    GameManager* m_gameManager;
    QVector<GoBigger::AI::SimpleAIPlayer*> m_monitoredAI;
    GoBigger::AI::SimpleAIPlayer* m_selectedAI;
    QTimer* m_refreshTimer;
    
    // 性能统计
    struct PerformanceStats {
        double fps = 0.0;
        double cpuUsage = 0.0;
        double memoryUsage = 0.0;
        int actionCount = 0;
        int decisionCount = 0;
    } m_perfStats;
    
    // 日志管理
    void addLogEntry(const QString& message, const QString& level = "INFO");
    void clearLogs();
    int m_maxLogEntries = 1000;
};
